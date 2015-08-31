#include "stdafx.h"
#include "File.h"
#include "Manager.h"
#include "Application.h"

#include <mono/metadata/mono-config.h>	// for mono_config_parse()
#include <mono/metadata/threads.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/mono-gc.h>
#include <mono/metadata/environment.h>
#include <mono/metadata/mono-debug.h>

#define ENABLE_SOFT_DEBUGGING

// icalls
static void reload() {
	printf("Reloading\n");
	auto app = Manager::GetApplication();
	app->FireOnReload();
}

static MonoMethod *find_method(MonoClass *klass, const char *name) {
	MonoMethod *method = mono_class_get_method_from_name(klass, name, -1);
	if (!method) {
		return NULL;
	}

	return method;
}

static MonoMethod *find_method(MonoImage *image, const char *className, const char *namspace, const char *methodName) {
	MonoClass *klass = mono_class_from_name(image, namspace, className);
	if (!klass)	{
		return NULL;
	}

	return find_method(klass, methodName);
}

static MonoDomain *load_domain() {
	MonoDomain *newDomain = mono_domain_create_appdomain("MyAppDomain", NULL);
	if (!newDomain) {
		printf("Error creating domain\n");
		return nullptr;
	}

	//mono_thread_push_appdomain_ref(newDomain);

	if (!mono_domain_set(newDomain, false)) {
		printf("Error setting domain\n");
		return nullptr;
	}

	mono_thread_attach(newDomain);

	return mono_domain_get();
}

static void unload_domain() {
	MonoDomain *old_domain = mono_domain_get();
	if (old_domain && old_domain != mono_get_root_domain()) {
		if (!mono_domain_set(mono_get_root_domain(), false)) {
			printf("Error setting domain\n");
		}

		//mono_thread_pop_appdomain_ref();
		mono_domain_unload(old_domain);
	}

	// unloading a domain is also a nice point in time to have the GC run.
	mono_gc_collect(mono_gc_max_generation());
}

void Application::Init() {
	// root of the mono dir
	// you should set environment variable MONO_DIR
	// typically in Windows "C:\\Program Files(x86)\\Mono";
	std::string monoDir = std::getenv("MONO_DIR");

	std::string libmonoDir = File::BuildPath(monoDir, "lib");
	std::string etcmonoDir = File::BuildPath(monoDir, "etc");

	mono_set_dirs(libmonoDir.c_str(), etcmonoDir.c_str());

	InitializeMono();
}

void Application::InitializeMono() {
	// where we find user code
	assemblyDir = "Managed";
	assemblyFile = "ScriptApp.exe";

	//this will override the internal assembly search logic.
	//do it in case you package mono in a different structure
	//mono_set_assemblies_path(assemblyDir.c_str());

	mono_config_parse(NULL);

#ifdef ENABLE_SOFT_DEBUGGING
	const char *options[] = {
		"--soft-breakpoints",
		"--debugger-agent=transport=dt_socket,address=127.0.0.1:10000,server=y"
	};

	mono_jit_parse_options(sizeof(options) / sizeof(char*), (char **)options);

	mono_debug_init(MONO_DEBUG_FORMAT_MONO);
#endif

	// initialize the root domain which will hold corlib and will always be alive
	MonoDomain *rootDomain = mono_jit_init_version("MyRootDomain", "v4.0.30319");

	// soft debugger needs this
	mono_thread_set_main(mono_thread_current());

	// add icalls
	mono_add_internal_call("ScriptApp.Program::reload", reload);
}

void Application::Run() {
	// this is going to block until managed code exits
	while (!StartMonoAndLoadAssemblies()) {
		;
	}

	// unload the child domain
	unload_domain();

	// we're exiting the whole thing, cleanup
	mono_jit_cleanup(mono_domain_get());
}

bool Application::StartMonoAndLoadAssemblies() {
	// shutdown the child domain
	StopMono();

	// create a new child domain
	if (!StartMono()) {
		mono_environment_exitcode_set(-1);
		return true;
	}

	// start to load assembly in child domain
	std::string assemblyPath = File::BuildRootedPath(assemblyDir, assemblyFile);
	printf("Loading %s into Domain\n", assemblyFile.c_str());

	// read our entry point assembly
	size_t length;
	char *data = File::Read(assemblyPath.c_str(), &length);

	// open the assembly from the data we read, so we never lock files
	MonoImageOpenStatus status;
	auto image = mono_image_open_from_data_with_name(data, length, true/* copy data */, &status, false/* ref only */, assemblyPath.c_str());
	if (status != MONO_IMAGE_OK || image == nullptr) {
		printf("Failed loading assembly %s\n", assemblyFile.c_str());
		return true;
	}

	// load the assembly
	auto assembly = mono_assembly_load_from_full(image, assemblyPath.c_str(), &status, false);
	if (status != MONO_IMAGE_OK || assembly == nullptr) {
		mono_image_close(image);
		printf("Failed loading assembly %s\n", assemblyFile.c_str());
		return true;
	}

	// save the image for lookups later and for cleaning up
	images.push_back(image);

	if (!assembly) {
		printf("Couldn't find assembly %s\n", assemblyFile.c_str());
		return true;
	}

	// locate the class we want to load
	MonoClass *klass = mono_class_from_name(image, "ScriptApp", "Program");
	if (klass == nullptr) {
		printf("Failed loading class %s\n", "ScriptApp.Program");
		return true;
	}

	// create the object 
	// only allocates the storage (doesn't run constructors)
	MonoObject *obj = mono_object_new(mono_domain_get(), klass);
	if (obj == nullptr) {
		printf("Failed loading class instance %s\n", "ScriptApp.Program");
		return true;
	}

	// initialize the class instance (runs default constructors)
	mono_runtime_object_init(obj);
	if (obj == nullptr) {
		printf("Failed initializing class instance %s\n", "ScriptApp.Program");
		return true;
	}

	// save the class instance for lookups later
	instances.push_back(obj);

	// find the Run() method
	auto method = find_method(klass, "Run");
	
	// call the Run method. This will block until the managed code decides to exit
	MonoObject *exception = NULL;
	MonoObject *result = mono_runtime_invoke(method, obj, NULL, &exception);
	if (exception) {
		printf("An exception was thrown\n");
		return true;
	}

	int val = *(int*)mono_object_unbox(result);

	// if the managed code returns with 0, it wants to exit completely
	if (val == 0) {
		return true;
	}
	return false;
}

bool Application::StartMono() {
	domain = load_domain();
	if (!domain) {
		printf("Error loading domain\n");
		return false;
	}
	return true;
}

void Application::StopMono() {
	/*for (auto &img : images) {
		mono_image_close(img);
	}*/
	instances.clear();
	images.clear();
	unload_domain();
}

void Application::FireOnReload() {
	for (auto &obj : instances) {
		auto klass = mono_object_get_class(obj);
		if (klass) {
			auto method = find_method(klass, "OnReload");
			if (method) {
				mono_runtime_invoke(method, obj, NULL, NULL);
			}
		}
	}
}
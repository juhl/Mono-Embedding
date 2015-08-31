#pragma once

#include <mono/jit/jit.h>

class Application {
public:
	void Init();
	void Run();
	void FireOnReload();

private:
	void InitializeMono();
	bool StartMonoAndLoadAssemblies();
	void StopMono();
	bool StartMono();

	std::string assemblyDir;
	std::string assemblyFile;
	MonoDomain *domain;
	std::vector<MonoImage *> images;
	std::vector<MonoObject *> instances;
};

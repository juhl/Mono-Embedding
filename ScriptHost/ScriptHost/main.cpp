#include "stdafx.h"
#include "Manager.h"
#include "Application.h"
#include "File.h"

static Manager manager;

int main(int argc, const char* argv[]) {
	TCHAR cwd[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, cwd);
	std::string cwdDir = cwd;
	std::string binDir = "\\..\\..\\Bin";
	std::string binPath = cwdDir + binDir;

	File::SetExecDir(binPath.c_str());
	
	auto app = new Application();
	Manager::RegisterApplication(app);
	app->Init();
	app->Run();

	return 0;
}
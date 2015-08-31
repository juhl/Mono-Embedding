#pragma once

#include "stdafx.h"

class Application;

class Manager {
public:
	static void RegisterApplication(Application *app) { application = app; }
	static Application *GetApplication() { return application; }

private:
	static Application *application;
};
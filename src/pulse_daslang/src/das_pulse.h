#pragma once

#include "daScript/daScript.h"

namespace das
{
	class ModulePulse : public Module
	{
	public:
		ModulePulse();
		bool initDependencies();

	private:
		ModuleLibrary lib;
		bool initialized = false;
	};
}

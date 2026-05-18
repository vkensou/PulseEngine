// this file is generated via Daslang automatic binder
// all user modifications will be lost after this file is re-generated

#pragma once
namespace das {
class Module_dasCGPU : public Module {
public:
	Module_dasCGPU();
protected:
virtual bool initDependencies() override;
	void initMain ();
	virtual ModuleAotType aotRequire ( TextWriter & tw ) const override;
	#include "dasCGPU.func.decl.inc"
public:
	ModuleLibrary lib;
	bool initialized = false;
};
}


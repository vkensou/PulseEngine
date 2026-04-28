// this file is generated via Daslang automatic binder
// all user modifications will be lost after this file is re-generated

#include "daScript/misc/platform.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/simulate/bind_enum.h"
#include "dasCGPU.h"
#include "need_dasCGPU.h"
#include "dasCGPU.struct.impl.inc"
namespace das {
#include "dasCGPU.enum.class.inc"
#include "dasCGPU.struct.class.inc"
#include "dasCGPU.func.aot.inc"
Module_dasCGPU::Module_dasCGPU() : Module("cgpu") {
}
bool Module_dasCGPU::initDependencies() {
	if ( initialized ) return true;
	initialized = true;
	lib.addModule(this);
	lib.addBuiltInModule();
	#include "dasCGPU.enum.add.inc"
	#include "dasCGPU.dummy.add.inc"
	#include "dasCGPU.struct.add.inc"
	#include "dasCGPU.struct.postadd.inc"
	#include "dasCGPU.alias.add.inc"
	#include "dasCGPU.func.reg.inc"
	initMain();
	return true;
}
}
REGISTER_MODULE_IN_NAMESPACE(Module_dasCGPU,das);


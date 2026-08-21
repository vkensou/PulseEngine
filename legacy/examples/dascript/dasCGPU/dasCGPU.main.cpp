#include "daScript/misc/platform.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "dasCGPU.h"

namespace das {
    void Module_dasCGPU::initMain () {

    }
    ModuleAotType Module_dasCGPU::aotRequire ( TextWriter & tw ) const {
        return ModuleAotType::cpp;
    }
}
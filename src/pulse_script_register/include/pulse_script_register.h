#pragma once

// ============================================================================
// pulse_script_register - 脚本包运行时协议
//
// pulse_package_loader 与脚本引擎包（如 pulse_daslang）共享的类型定义。
// 引擎包 DLL 导出固定符号 PULSE_PACKAGE_GET_RUNTIMES_SYMBOL，loader 加载该
// 包后自动探测并登记运行时；package.json 中 "type" 非 native 的包按 type
// 分发给对应运行时 handler。
// ============================================================================

#include <stdint.h>

#include "pulse_app.h"
#include "pulse_config.h"

// loader 传给脚本运行时 handler 的包信息
typedef struct PulsePackageScriptInfo {
    const char* name;         // 包名
    const char* package_dir;  // 包目录（manifest 所在目录）
    const char* script_file;  // manifest 中 script_file 原值（相对包目录）
    PulseConfig* config;      // 宿主传入的配置，可为空
} PulsePackageScriptInfo;

typedef EPulseResult (*PulseProcScriptPackageLoadFn)(PulseAppId app, const PulsePackageScriptInfo* info);

typedef struct PulseScriptRuntimeDesc {
    const char* type;                    // 运行时类型名，如 "daslang"
    PulseProcScriptPackageLoadFn load;   // handler
} PulseScriptRuntimeDesc;

// 引擎包导出符号名。签名：返回运行时数量，*out_runtimes 指向静态数组。
#define PULSE_PACKAGE_GET_RUNTIMES_SYMBOL "pulse_package_get_runtimes"

typedef uint32_t (*PulseProcPackageGetRuntimesFn)(const PulseScriptRuntimeDesc** out_runtimes);

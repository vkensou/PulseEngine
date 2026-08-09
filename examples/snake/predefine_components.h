#pragma once

// ============================================================
// 游戏组件定义
//
// 资源加载使用原生 pulse-asset（异步 request → ready → handle）。
// main 只负责发起请求并填入 SnakeAssets 单例，就绪后的初始化
// 由游戏模块内的 prepareGameSystem 完成。
// ============================================================

#include <flecs.h>
#include <vector>
#include <memory>

#include "pulse_app.h"
#include "pulse_math.h"
#include "pulse_graphics.h"
#include "pulse_transform.h"
#include "pulse_renderer.h"
#include "ecsext.hpp"

// ============================================================
// ECS 资源（单例）
// ============================================================

// 资源请求容器：main 发起异步加载请求后填入；模块内系统等待 ready。
PULSE_ECS_RESOURCE
struct SnakeAssets
{
	PulseAppId app;
	PulseShaderRequest shader;
	PulseMeshRequest mesh;
	bool ready = false;
};

// ============================================================
// ECS 组件（游戏逻辑用）
// ============================================================


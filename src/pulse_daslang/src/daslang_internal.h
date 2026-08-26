#pragma once
#include <memory>
#include <string>
#include <vector>

#include "pulse_daslang.h"
#include "pulse_asset.h"
#include "daScript/daScript.h"
#include "daScript/simulate/fs_file_info.h"

namespace pulse_daslang_internal
{
	constexpr const char* kPluginName = "pulse_daslang";

	struct DaslangScriptText;

	struct Module
	{
		Module() = default;
		Module(std::string path, const DaslangScriptText* text, das::FileInfoPtr main_info, das::ModuleGroup group, das::ProgramPtr prog, std::unique_ptr<das::Context> ctx)
			: script_path(std::move(path))
			, script_text(text)
			, main_file_info(std::move(main_info))
			, module_group(std::move(group))
			, program(std::move(prog))
			, context(std::move(ctx))
		{
		}

		Module(const Module&) = delete;
		Module& operator=(const Module&) = delete;
		Module(Module&&) = default;
		Module& operator=(Module&&) = default;
		~Module();

		std::string script_path;
		// 脚本文本 asset 的借用指针：脚本资源驻留 asset 缓存、从不 release，
		// shutdown 时先清空 modules 再卸载 asset，因此该引用在整个模块生命周期
		// 内稳定（daScript 的 FileInfo/LineInfo 也直接引用这份文本）。
		const DaslangScriptText* script_text = nullptr;
		// 主脚本的 FileInfo：编译时注入共享 FileAccess（state->access），
		// 编译完成后由 letGoOfFileInfo 移出、交本模块持有——若留在共享 map
		// 里，同路径重编译时 setFileInfo 覆盖会销毁旧 FileInfo，导致已编译
		// 模块悬垂。标准库的 FileInfo 保留在共享 access 中（首编译打开一次，
		// 跨编译 map 命中复用），其生命周期覆盖所有模块（shutdown 时先清
		// modules 再销毁 access）。
		das::FileInfoPtr main_file_info;
		das::ModuleGroup module_group;
		das::ProgramPtr program;
		std::unique_ptr<das::Context> context;
	};

	// 脚本资源在 pulse_asset 里的载荷：纯文本。loader 只负责把文件字节拷进来。
	struct DaslangScriptText
	{
		std::string text;
	};

	// 一个已经提交给 pulse_asset 的脚本加载请求（异步，等待加载完成后编译）。
	struct PendingScript
	{
		std::string path;
		PulseAssetRequest request;
	};

	struct pulse_daslang_state
	{
		PulseAppId app = nullptr;
		PulseAssetSystemId asset_system = nullptr;
		// das 标准库根目录（包内相对路径），例如 "das"（含 daslib/ 与 pulse/）。
		std::string das_root;
		// 共享的编译文件访问：所有脚本（主脚本 + daslib/pulse 标准库）的文件
		// 读取都经它走 pulse_asset（DasAssetFileSystem），标准库 FileInfo 首
		// 编译打开一次后跨编译复用。生命周期覆盖所有 Module。
		das::smart_ptr<das::FsFileAccess> access;
		std::vector<Module> modules;
		std::vector<PendingScript> pending_scripts;
		ecs_entity_t process_system = 0;
	};

	class DaslangTextPrinter : public das::TextWriter
	{
	public:
		virtual void output() override
		{
			uint64_t new_pos = tellp();
			if (new_pos != pos)
			{
				fwrite(data() + pos, 1, static_cast<size_t>(new_pos - pos), stdout);
				fflush(stdout);
				pos = new_pos;
			}
		}

	protected:
		uint64_t pos = 0;
	};

	// 向 pulse_asset 提交一个脚本加载请求并登记到 state（按路径去重）。
	bool request_script_load(PulseAppId app, pulse_daslang_state* state, const char* script_path);

	// 处理所有已加载完成的脚本请求：borrow 文本 -> 编译 -> simulate ->
	// 执行 importModule。文本 asset 驻留缓存不 release（模块直接引用其文本）。
	// PostBuild 与运行时系统共用。
	void process_pending_scripts(pulse_daslang_state* state);
}

typedef struct pulse_daslang_state_resource {
    pulse_daslang_internal::pulse_daslang_state* state;
} pulse_daslang_state_resource;
extern ECS_COMPONENT_DECLARE(pulse_daslang_state_resource);

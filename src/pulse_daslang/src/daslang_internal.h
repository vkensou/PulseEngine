#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "pulse_daslang.h"
#include "daScript/daScript.h"
#include "daScript/simulate/fs_file_info.h"

namespace pulse_daslang_internal
{
	constexpr const char* kPluginName = "pulse_daslang";

	struct Module
	{
		Module() = default;
		Module(std::string path, das::FileInfoPtr main_info, das::ModuleGroup group, das::ProgramPtr prog, std::unique_ptr<das::Context> ctx)
			: script_path(std::move(path))
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
		das::FileInfoPtr main_file_info;
		das::ModuleGroup module_group;
		das::ProgramPtr program;
		std::unique_ptr<das::Context> context;
	};

	struct pulse_daslang_state
	{
		PulseAppId app = nullptr;
		std::string das_root;
		das::smart_ptr<das::FsFileAccess> access;
		std::vector<Module> modules;
		std::unordered_map<std::string, std::string> script_cache;
		std::vector<std::string> pending_scripts;
        ecs_entity_t process_system = 0;
		bool post_build_done = false;
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

	std::string normalize_cache_key(std::string_view path);

	bool compile_entry_from_cache(pulse_daslang_state* state, const std::string& key);
}

typedef struct pulse_daslang_state_resource {
    pulse_daslang_internal::pulse_daslang_state* state;
} pulse_daslang_state_resource;
extern ECS_COMPONENT_DECLARE(pulse_daslang_state_resource);

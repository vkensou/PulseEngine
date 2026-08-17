#pragma once
#include <memory>
#include <string>
#include <vector>

#include "pulse_daslang.h"
#include "daScript/daScript.h"

namespace pulse_daslang_internal
{
	constexpr const char* kPluginName = "PulseDaslangPlugin";

    struct Module
    {
        Module() = default;
        Module(std::string path, das::ModuleGroup group, das::ProgramPtr prog, std::unique_ptr<das::Context> ctx)
            : script_path(std::move(path))
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
		das::ModuleGroup module_group;
		das::ProgramPtr program;
		std::unique_ptr<das::Context> context;
    };

	struct pulse_daslang_state
	{
		PulseAppId app = nullptr;
		std::string root_path;
        std::vector<Module> modules;
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
}

typedef struct pulse_daslang_state_resource {
    pulse_daslang_internal::pulse_daslang_state* state;
} pulse_daslang_state_resource;
extern ECS_COMPONENT_DECLARE(pulse_daslang_state_resource);

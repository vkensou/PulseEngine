#include "das_pulse.h"

#include <cstring>

#include "daScript/misc/platform.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/simulate/bind_enum.h"

#include "das_flecs.h"

#include <imgui.h>

#include "pulse_app.h"
#include "pulse_asset.h"
#include "pulse_graphics.h"
#include "pulse_math.h"

// ============================================================
// Opaque handle wrappers
//
// PulseAppId / PulseAssetSystemId are pointer typedefs. daScript value
// annotations need a trivially copyable C++ struct to bind by value, so the
// handles are boxed; the daslang type names stay PulseAppId /
// PulseAssetSystemId.
// ============================================================

struct PulseAppHandle
{
	PulseAppId app;
};

struct PulseAssetSystemHandle
{
	PulseAssetSystemId asset_system;
};

// ============================================================
// API shim wrappers (static-inline helpers + handle boxing)
// ============================================================

static PulseAssetRequest das_shader_request_to_asset_request(const PulseShaderRequest& shader)
{
	return pulse_shader_request_to_asset_request(shader);
}

static PulseAssetRequest das_mesh_request_to_asset_request(const PulseMeshRequest& mesh)
{
	return pulse_mesh_request_to_asset_request(mesh);
}

static PulseAppHandle das_get_app_from_world(dasPulseECS::World& world)
{
	return { pulse_get_app_from_world(world.world_) };
}

static PulseAssetSystemHandle das_get_asset_system(const PulseAppHandle& app)
{
	return { pulse_get_asset_system(app.app) };
}

static EPulseAssetState das_asset_system_get_state(const PulseAssetSystemHandle& asset_system, const PulseAssetRequest& request)
{
	return pulse_asset_system_get_state(asset_system.asset_system, request);
}

static const char* das_asset_system_get_error(const PulseAssetSystemHandle& asset_system, const PulseAssetRequest& request)
{
	return pulse_asset_system_get_error(asset_system.asset_system, request);
}

static PulseShaderRequest das_load_shader(const PulseAppHandle& app, const char* filepath)
{
	return pulse_load_shader(app.app, filepath);
}

static PulseMeshRequest das_load_mesh(const PulseAppHandle& app, const char* filepath)
{
	return pulse_load_mesh(app.app, filepath);
}

static bool das_shader_is_ready(const PulseAppHandle& app, const PulseShaderRequest& request)
{
	return pulse_shader_is_ready(app.app, request);
}

static bool das_mesh_is_ready(const PulseAppHandle& app, const PulseMeshRequest& request)
{
	return pulse_mesh_is_ready(app.app, request);
}

static PulseShaderHandle das_shader_get_handle(const PulseAppHandle& app, const PulseShaderRequest& request)
{
	return pulse_shader_get_handle(app.app, request);
}

static PulseMeshHandle das_mesh_get_handle(const PulseAppHandle& app, const PulseMeshRequest& request)
{
	return pulse_mesh_get_handle(app.app, request);
}

static PulseMaterialHandle das_create_material(const PulseAppHandle& app, const PulseMaterialCreateDesc* desc)
{
	return pulse_create_material(app.app, desc);
}

static void das_material_set_property_float4(const PulseAppHandle& app, const PulseMaterialHandle& material, const char* name, const HMM_Vec4& value)
{
	pulse_material_set_property_float4(app.app, material, name, value);
}

static void das_text(const char* txt)
{
	ImGui::Text("%s", txt);
}

static bool das_button(const char* label)
{
	return ImGui::Button(label);
}

// ============================================================
// Math type annotations
// ============================================================

MAKE_TYPE_FACTORY(HMM_Vec3, HMM_Vec3);
struct HMM_Vec3Annotation final : das::ManagedStructureAnnotation<HMM_Vec3>
{
	HMM_Vec3Annotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("HMM_Vec3", ml, "HMM_Vec3")
	{
		addField<DAS_BIND_MANAGED_FIELD(X)>("X");
		addField<DAS_BIND_MANAGED_FIELD(Y)>("Y");
		addField<DAS_BIND_MANAGED_FIELD(Z)>("Z");
	}

	virtual bool isLocal() const override { return true; }
};

MAKE_TYPE_FACTORY(HMM_Vec4, HMM_Vec4);
struct HMM_Vec4Annotation final : das::ManagedStructureAnnotation<HMM_Vec4>
{
	HMM_Vec4Annotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("HMM_Vec4", ml, "HMM_Vec4")
	{
		addField<DAS_BIND_MANAGED_FIELD(X)>("X");
		addField<DAS_BIND_MANAGED_FIELD(Y)>("Y");
		addField<DAS_BIND_MANAGED_FIELD(Z)>("Z");
		addField<DAS_BIND_MANAGED_FIELD(W)>("W");
	}

	virtual bool isLocal() const override { return true; }
};

MAKE_TYPE_FACTORY(HMM_Mat4, HMM_Mat4);
struct HMM_Mat4Annotation final : das::ManagedStructureAnnotation<HMM_Mat4>
{
	HMM_Mat4Annotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("HMM_Mat4", ml, "HMM_Mat4")
	{
		addField<DAS_BIND_MANAGED_FIELD(Columns)>("Columns");
	}

	virtual bool isLocal() const override { return true; }
};

MAKE_TYPE_FACTORY(HMM_Quat, HMM_Quat);
struct HMM_QuatAnnotation final : das::ManagedStructureAnnotation<HMM_Quat>
{
	HMM_QuatAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("HMM_Quat", ml, "HMM_Quat")
	{
		addField<DAS_BIND_MANAGED_FIELD(X)>("X");
		addField<DAS_BIND_MANAGED_FIELD(Y)>("Y");
		addField<DAS_BIND_MANAGED_FIELD(Z)>("Z");
		addField<DAS_BIND_MANAGED_FIELD(W)>("W");
	}

	virtual bool isLocal() const override { return true; }
};

static inline void HMM_TRS_bind(const HMM_Vec3& translation, const HMM_Quat& rotation, const HMM_Vec3& scale, HMM_Mat4& out_matrix)
{
	out_matrix = HMM_TRS(translation, rotation, scale);
}

MAKE_TYPE_FACTORY(EventTag, pulse::EventTag);
MAKE_TYPE_FACTORY(PulseAppId, PulseAppHandle);
MAKE_TYPE_FACTORY(PulseAssetSystemId, PulseAssetSystemHandle);

struct PulseAppIdAnnotation final : das::ManagedStructureAnnotation<PulseAppHandle>
{
	PulseAppIdAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("PulseAppId", ml, "PulseAppHandle")
	{
	}
};

struct PulseAssetSystemIdAnnotation final : das::ManagedStructureAnnotation<PulseAssetSystemHandle>
{
	PulseAssetSystemIdAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("PulseAssetSystemId", ml, "PulseAssetSystemHandle")
	{
	}
};
struct EventTagAnnotation final : das::ManagedStructureAnnotation<pulse::EventTag>
{
	EventTagAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("EventTag", ml, "pulse::EventTag")
	{
	}
};

// ============================================================
// Asset / graphics value struct annotations
// ============================================================

MAKE_TYPE_FACTORY(PulseShaderRequest, PulseShaderRequest);
struct PulseShaderRequestAnnotation final : das::ManagedStructureAnnotation<PulseShaderRequest>
{
	PulseShaderRequestAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("PulseShaderRequest", ml, "PulseShaderRequest")
	{
		addField<DAS_BIND_MANAGED_FIELD(index)>("index");
		addField<DAS_BIND_MANAGED_FIELD(generation)>("generation");
	}
};

MAKE_TYPE_FACTORY(PulseMeshRequest, PulseMeshRequest);
struct PulseMeshRequestAnnotation final : das::ManagedStructureAnnotation<PulseMeshRequest>
{
	PulseMeshRequestAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("PulseMeshRequest", ml, "PulseMeshRequest")
	{
		addField<DAS_BIND_MANAGED_FIELD(index)>("index");
		addField<DAS_BIND_MANAGED_FIELD(generation)>("generation");
	}
};

MAKE_TYPE_FACTORY(PulseShaderHandle, PulseShaderHandle);
struct PulseShaderHandleAnnotation final : das::ManagedStructureAnnotation<PulseShaderHandle>
{
	PulseShaderHandleAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("PulseShaderHandle", ml, "PulseShaderHandle")
	{
		addField<DAS_BIND_MANAGED_FIELD(index)>("index");
		addField<DAS_BIND_MANAGED_FIELD(generation)>("generation");
	}
};

MAKE_TYPE_FACTORY(PulseMeshHandle, PulseMeshHandle);
struct PulseMeshHandleAnnotation final : das::ManagedStructureAnnotation<PulseMeshHandle>
{
	PulseMeshHandleAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("PulseMeshHandle", ml, "PulseMeshHandle")
	{
		addField<DAS_BIND_MANAGED_FIELD(index)>("index");
		addField<DAS_BIND_MANAGED_FIELD(generation)>("generation");
	}
};

MAKE_TYPE_FACTORY(PulseMaterialHandle, PulseMaterialHandle);
struct PulseMaterialHandleAnnotation final : das::ManagedStructureAnnotation<PulseMaterialHandle>
{
	PulseMaterialHandleAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("PulseMaterialHandle", ml, "PulseMaterialHandle")
	{
		addField<DAS_BIND_MANAGED_FIELD(index)>("index");
		addField<DAS_BIND_MANAGED_FIELD(generation)>("generation");
	}
};

MAKE_TYPE_FACTORY(PulseAssetRequest, PulseAssetRequest);
struct PulseAssetRequestAnnotation final : das::ManagedStructureAnnotation<PulseAssetRequest>
{
	PulseAssetRequestAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("PulseAssetRequest", ml, "PulseAssetRequest")
	{
		addField<DAS_BIND_MANAGED_FIELD(type_id)>("type_id");
		addField<DAS_BIND_MANAGED_FIELD(index)>("index");
		addField<DAS_BIND_MANAGED_FIELD(generation)>("generation");
	}
};

MAKE_TYPE_FACTORY(PulseMaterialCreateDesc, PulseMaterialCreateDesc);
struct PulseMaterialCreateDescAnnotation final : das::ManagedStructureAnnotation<PulseMaterialCreateDesc>
{
	PulseMaterialCreateDescAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("PulseMaterialCreateDesc", ml, "PulseMaterialCreateDesc")
	{
		addField<DAS_BIND_MANAGED_FIELD(shader)>("shader");
	}
};

// ============================================================
// Shader descriptor annotations
// ============================================================

DAS_BASE_BIND_ENUM(EPulseAssetState, EPulseAssetState,
	PULSE_ASSET_STATE_EMPTY,
	PULSE_ASSET_STATE_WAITING_LOAD,
	PULSE_ASSET_STATE_LOADING,
	PULSE_ASSET_STATE_WAITING_DEPENDENCIES,
	PULSE_ASSET_STATE_PROCESSING,
	PULSE_ASSET_STATE_LOADED,
	PULSE_ASSET_STATE_FAILED,
	PULSE_ASSET_STATE_PENDING_DELETE);

DAS_BIND_ENUM_CAST(EPulseAssetState);

namespace das
{
	template <>
	struct cast<HMM_Quat>
	{
		static __forceinline HMM_Quat to(vec4f x) { return HMM_Q(v_extract_x(x), v_extract_y(x), v_extract_z(x), v_extract_w(x)); }
		static __forceinline vec4f from(HMM_Quat x) { return v_make_vec4f(x.X, x.Y, x.Z, x.W); }
	};
	template <> struct WrapType<HMM_Quat> { enum { value = true }; typedef HMM_Quat type; typedef HMM_Quat rettype; };
}

#define DAS_PULSE_VALUE_CAST(TYPE)                                                                 \
	namespace das                                                                                \
	{                                                                                             \
		template <> struct cast<TYPE>                                                             \
		{                                                                                         \
			static __forceinline TYPE to(vec4f x)                                                 \
			{                                                                                     \
				TYPE value;                                                                       \
				memcpy(&value, &x, sizeof(TYPE));                                                 \
				return value;                                                                     \
			}                                                                                     \
			static __forceinline vec4f from(const TYPE& value)                                    \
			{                                                                                     \
				vec4f result;                                                                     \
				memset(&result, 0, sizeof(result));                                               \
				memcpy(&result, &value, sizeof(TYPE));                                            \
				return result;                                                                    \
			}                                                                                     \
		};                                                                                        \
	}

DAS_PULSE_VALUE_CAST(HMM_Vec4);
DAS_PULSE_VALUE_CAST(PulseAppHandle);
DAS_PULSE_VALUE_CAST(PulseAssetSystemHandle);
DAS_PULSE_VALUE_CAST(PulseShaderRequest);
DAS_PULSE_VALUE_CAST(PulseMeshRequest);
DAS_PULSE_VALUE_CAST(PulseShaderHandle);
DAS_PULSE_VALUE_CAST(PulseMeshHandle);
DAS_PULSE_VALUE_CAST(PulseMaterialHandle);
DAS_PULSE_VALUE_CAST(PulseAssetRequest);

// ============================================================
// pulse module
// ============================================================

namespace das
{
	bool ModulePulse::initDependencies()
	{
		if (initialized)
			return true;
		initialized = true;

		lib.addModule(this);
		lib.addBuiltInModule();
		lib.addModule(Module::require("flecs"));

		addAnnotation(new HMM_Vec3Annotation(lib));
		addAnnotation(new HMM_Vec4Annotation(lib));
		addAnnotation(new HMM_Mat4Annotation(lib));
		addAnnotation(new HMM_QuatAnnotation(lib));
		addAnnotation(new EventTagAnnotation(lib));
		addAnnotation(new PulseAppIdAnnotation(lib));
		addAnnotation(new PulseAssetSystemIdAnnotation(lib));

		addEnumeration(new ::EnumerationEPulseAssetState());

		addAnnotation(new PulseShaderRequestAnnotation(lib));
		addAnnotation(new PulseMeshRequestAnnotation(lib));
		addAnnotation(new PulseShaderHandleAnnotation(lib));
		addAnnotation(new PulseMeshHandleAnnotation(lib));
		addAnnotation(new PulseMaterialHandleAnnotation(lib));
		addAnnotation(new PulseAssetRequestAnnotation(lib));
		addAnnotation(new PulseMaterialCreateDescAnnotation(lib));

		addExtern<DAS_BIND_FUN(HMM_V3), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "HMM_V3", SideEffects::none, "HMM_V3")->args({ "x", "y", "z" });
		addExtern<DAS_BIND_FUN(HMM_V4), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "HMM_V4", SideEffects::none, "HMM_V4")->args({ "x", "y", "z", "w" });
		addExtern<DAS_BIND_FUN(HMM_TRS_bind)>(*this, lib, "HMM_TRS", SideEffects::modifyArgument, "HMM_TRS")->args({ "translation", "rotation", "scale", "out" });

		addExtern<DAS_BIND_FUN(das_get_app_from_world), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "pulse_get_app_from_world", SideEffects::modifyExternal, "pulse_get_app_from_world")->args({ "world" });

		addExtern<DAS_BIND_FUN(das_shader_request_to_asset_request), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "pulse_shader_request_to_asset_request", SideEffects::none, "pulse_shader_request_to_asset_request")->args({ "request" });
		addExtern<DAS_BIND_FUN(das_mesh_request_to_asset_request), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "pulse_mesh_request_to_asset_request", SideEffects::none, "pulse_mesh_request_to_asset_request")->args({ "request" });

		addExtern<DAS_BIND_FUN(das_get_asset_system), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "pulse_get_asset_system", SideEffects::modifyExternal, "pulse_get_asset_system")->args({ "app" });
		addExtern<DAS_BIND_FUN(das_asset_system_get_state)>(*this, lib, "pulse_asset_system_get_state", SideEffects::modifyExternal, "pulse_asset_system_get_state")->args({ "asset_system", "request" });
		addExtern<DAS_BIND_FUN(das_asset_system_get_error)>(*this, lib, "pulse_asset_system_get_error", SideEffects::modifyExternal, "pulse_asset_system_get_error")->args({ "asset_system", "request" });

		addExtern<DAS_BIND_FUN(das_load_shader), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "pulse_load_shader", SideEffects::worstDefault, "pulse_load_shader")->args({ "app", "path" });
		addExtern<DAS_BIND_FUN(das_load_mesh), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "pulse_load_mesh", SideEffects::worstDefault, "pulse_load_mesh")->args({ "app", "path" });
		addExtern<DAS_BIND_FUN(das_shader_is_ready)>(*this, lib, "pulse_shader_is_ready", SideEffects::modifyExternal, "pulse_shader_is_ready")->args({ "app", "request" });
		addExtern<DAS_BIND_FUN(das_mesh_is_ready)>(*this, lib, "pulse_mesh_is_ready", SideEffects::modifyExternal, "pulse_mesh_is_ready")->args({ "app", "request" });
		addExtern<DAS_BIND_FUN(das_shader_get_handle), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "pulse_shader_get_handle", SideEffects::modifyExternal, "pulse_shader_get_handle")->args({ "app", "request" });
		addExtern<DAS_BIND_FUN(das_mesh_get_handle), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "pulse_mesh_get_handle", SideEffects::modifyExternal, "pulse_mesh_get_handle")->args({ "app", "request" });
		addExtern<DAS_BIND_FUN(das_create_material), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "pulse_create_material", SideEffects::worstDefault, "pulse_create_material")->args({ "app", "desc" });
		addExtern<DAS_BIND_FUN(das_material_set_property_float4)>(*this, lib, "pulse_material_set_property_float4", SideEffects::modifyExternal, "pulse_material_set_property_float4")->args({ "app", "material", "name", "value" });

		addExtern<DAS_BIND_FUN(das_text)>(*this, lib, "Text", SideEffects::worstDefault, "Text")->args({ "txt" });
		addExtern<DAS_BIND_FUN(das_button)>(*this, lib, "Button", SideEffects::worstDefault, "Button")->args({ "label" });

		return true;
	}

	ModulePulse::ModulePulse()
		: Module("pulse")
	{
	}
}

REGISTER_MODULE_IN_NAMESPACE(ModulePulse, das);

#include "das_pulse.h"

#include <cstring>
#include <string_view>

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
#include "pulse_datatable.h"
#include "pulse_graphics.h"
#include "pulse_math.h"
#include "pulse_prefab.h"

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

static PulseAssetRequest das_material_request_to_asset_request(const PulseMaterialRequest& material)
{
	return pulse_material_request_to_asset_request(material);
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

static PulseMaterialRequest das_load_material(const PulseAppHandle& app, const char* filepath)
{
	return pulse_load_material(app.app, filepath);
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

static bool das_material_is_ready(const PulseAppHandle& app, const PulseMaterialRequest& request)
{
	return pulse_material_is_ready(app.app, request);
}

static PulseMaterialHandle das_material_get_handle(const PulseAppHandle& app, const PulseMaterialRequest& request)
{
	return pulse_material_get_handle(app.app, request);
}

static PulseMaterialHandle das_create_material(const PulseAppHandle& app, const PulseMaterialCreateDesc* desc)
{
	return pulse_create_material(app.app, desc);
}

static PulseAssetRequest das_prefab_request_to_asset_request(const PulsePrefabRequest& prefab)
{
	return pulse_prefab_request_to_asset_request(prefab);
}

static PulsePrefabRequest das_load_prefab(const PulseAppHandle& app, const char* filepath)
{
	return pulse_load_prefab(app.app, filepath);
}

static bool das_prefab_is_ready(const PulseAppHandle& app, const PulsePrefabRequest& request)
{
	return pulse_prefab_is_ready(app.app, request);
}

static PulsePrefabHandle das_prefab_get_handle(const PulseAppHandle& app, const PulsePrefabRequest& request)
{
	return pulse_prefab_get_handle(app.app, request);
}

static ecs_entity_t das_prefab_instantiate(const PulseAppHandle& app, const PulsePrefabHandle& prefab)
{
	return pulse_prefab_instantiate(app.app, prefab);
}

static void das_material_set_property_float4(const PulseAppHandle& app, const PulseMaterialHandle& material, const char* name, const HMM_Vec4& value)
{
	pulse_material_set_property_float4(app.app, material, name, value);
}

struct DasDataTableField
{
	const PulseDataTableColumnDesc* column = nullptr;
	uint32_t offset = 0;
};

static const PulseDataTableStructDesc* das_data_table_struct(const PulseDataTableSchemaDesc* schema, const char* name)
{
	if (!schema || !name) {
		return nullptr;
	}
	for (uint32_t i = 0; i < schema->structs_count; ++i) {
		if (schema->p_structs[i].name && std::strcmp(schema->p_structs[i].name, name) == 0) {
			return &schema->p_structs[i];
		}
	}
	return nullptr;
}

static bool das_data_table_resolve(const PulseDataTableSchemaDesc* schema, int64_t c0, int64_t c1, int64_t c2, int64_t c3, DasDataTableField& out)
{
	if (!schema || c0 < 0 || static_cast<uint64_t>(c0) >= schema->columns_count) {
		return false;
	}
	const PulseDataTableColumnDesc* column = &schema->p_columns[c0];
	uint32_t base = 0;
	const int64_t path[3] = { c1, c2, c3 };
	for (int i = 0; i < 3; ++i) {
		if (path[i] < 0) {
			break;
		}
		if (column->type != PULSE_DATA_TABLE_COLUMN_TYPE_STRUCT || !column->struct_type) {
			return false;
		}
		const PulseDataTableStructDesc* nested = das_data_table_struct(schema, column->struct_type);
		if (!nested || static_cast<uint64_t>(path[i]) >= nested->columns_count) {
			return false;
		}
		base += column->offset;
		column = &nested->p_columns[path[i]];
	}
	out.column = column;
	out.offset = base + column->offset;
	return true;
}

static const PulseDataTableSchemaDesc* das_data_table_schema(const PulseAppHandle& app, const char* schema)
{
	PulseDataTableSystemId system = pulse_get_data_table_system(app.app);
	return system ? pulse_data_table_system_get_schema(system, schema) : nullptr;
}

static char* das_data_table_register_schema(const PulseAppHandle& app, const PulseDataTableSchemaDesc& desc, das::Context* context, das::LineInfoArg* at)
{
	PulseDataTableSystemId system = pulse_get_data_table_system(app.app);
	if (!system) {
		const char* message = "data table system is not available";
		return context->allocateString(message, static_cast<uint32_t>(std::strlen(message)), at);
	}
	PulseDataTableSchemaDesc owned = desc;
	owned.struct_size = sizeof(PulseDataTableSchemaDesc);
	owned.version = PULSE_DATA_TABLE_PLUGIN_DESC_VERSION;
	owned.fill_row = nullptr;
	const char* error = nullptr;
	if (pulse_data_table_system_register_schema(system, &owned, &error) != PULSE_RESULT_OK) {
		const char* message = error ? error : "data table schema could not be registered";
		return context->allocateString(message, static_cast<uint32_t>(std::strlen(message)), at);
	}
	return context->allocateString("", 0, at);
}

static PulseAssetRequest das_data_table_load(const PulseAppHandle& app, const char* schema, const char* path)
{
	PulseDataTableSystemId system = pulse_get_data_table_system(app.app);
	if (!system) {
		return pulse_asset_request_make_invalid();
	}
	return pulse_data_table_system_load(system, schema, path);
}

static bool das_data_table_is_ready(const PulseAppHandle& app, const PulseAssetRequest& request)
{
	PulseDataTableSystemId system = pulse_get_data_table_system(app.app);
	return system && pulse_data_table_system_is_ready(system, request);
}

static char* das_data_table_get_error(const PulseAppHandle& app, const PulseAssetRequest& request, das::Context* context, das::LineInfoArg* at)
{
	PulseDataTableSystemId system = pulse_get_data_table_system(app.app);
	const char* error = system ? pulse_data_table_system_get_error(system, request) : nullptr;
	if (!error) {
		return context->allocateString("", 0, at);
	}
	return context->allocateString(error, static_cast<uint32_t>(std::strlen(error)), at);
}

static PulseDataTableId das_data_table_table(const PulseAppHandle& app, const char* schema)
{
	PulseDataTableSystemId system = pulse_get_data_table_system(app.app);
	return system ? pulse_data_table_system_get_by_name(system, schema) : nullptr;
}

static int64_t das_data_table_row_count(const PulseAppHandle& app, const char* schema)
{
	return static_cast<int64_t>(pulse_data_table_row_count(das_data_table_table(app, schema)));
}

static void* das_data_table_row_at(const PulseAppHandle& app, const char* schema, int64_t index)
{
	if (index < 0) {
		return nullptr;
	}
	return const_cast<void*>(pulse_data_table_row_at(das_data_table_table(app, schema), static_cast<uint32_t>(index)));
}

static void* das_data_table_find_row(const PulseAppHandle& app, const char* schema, const char* key)
{
	return const_cast<void*>(pulse_data_table_find_row(das_data_table_table(app, schema), key));
}

static void* das_data_table_find_row_int(const PulseAppHandle& app, const char* schema, int64_t key)
{
	return const_cast<void*>(pulse_data_table_find_row_int(das_data_table_table(app, schema), key));
}

static const void* das_data_table_field(const PulseAppHandle& app, const char* schema, const void* row, int64_t c0, int64_t c1, int64_t c2, int64_t c3)
{
	if (!row) {
		return nullptr;
	}
	DasDataTableField field{};
	if (!das_data_table_resolve(das_data_table_schema(app, schema), c0, c1, c2, c3, field)) {
		return nullptr;
	}
	return static_cast<const char*>(row) + field.offset;
}

static int64_t das_data_table_read_int(const PulseAppHandle& app, const char* schema, const void* row, int64_t c0, int64_t c1, int64_t c2, int64_t c3)
{
	const void* address = das_data_table_field(app, schema, row, c0, c1, c2, c3);
	return address ? *static_cast<const int64_t*>(address) : 0;
}

static double das_data_table_read_float(const PulseAppHandle& app, const char* schema, const void* row, int64_t c0, int64_t c1, int64_t c2, int64_t c3)
{
	const void* address = das_data_table_field(app, schema, row, c0, c1, c2, c3);
	return address ? *static_cast<const double*>(address) : 0.0;
}

static bool das_data_table_read_bool(const PulseAppHandle& app, const char* schema, const void* row, int64_t c0, int64_t c1, int64_t c2, int64_t c3)
{
	const void* address = das_data_table_field(app, schema, row, c0, c1, c2, c3);
	return address ? *static_cast<const bool*>(address) : false;
}

static char* das_data_table_read_string(const PulseAppHandle& app, const char* schema, const void* row, int64_t c0, int64_t c1, int64_t c2, int64_t c3, das::Context* context, das::LineInfoArg* at)
{
	const void* address = das_data_table_field(app, schema, row, c0, c1, c2, c3);
	if (!address) {
		return context->allocateString("", 0, at);
	}
	const std::string_view* value = static_cast<const std::string_view*>(address);
	return context->allocateString(value->data(), static_cast<uint32_t>(value->size()), at);
}

static void* das_data_table_read_ref(const PulseAppHandle& app, const char* schema, const void* row, int64_t c0, int64_t c1, int64_t c2, int64_t c3)
{
	const void* address = das_data_table_field(app, schema, row, c0, c1, c2, c3);
	return address ? const_cast<void*>(*static_cast<const void* const*>(address)) : nullptr;
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

MAKE_TYPE_FACTORY(PulseMaterialRequest, PulseMaterialRequest);
struct PulseMaterialRequestAnnotation final : das::ManagedStructureAnnotation<PulseMaterialRequest>
{
	PulseMaterialRequestAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("PulseMaterialRequest", ml, "PulseMaterialRequest")
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

MAKE_TYPE_FACTORY(PulsePrefabRequest, PulsePrefabRequest);
struct PulsePrefabRequestAnnotation final : das::ManagedStructureAnnotation<PulsePrefabRequest>
{
	PulsePrefabRequestAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("PulsePrefabRequest", ml, "PulsePrefabRequest")
	{
		addField<DAS_BIND_MANAGED_FIELD(index)>("index");
		addField<DAS_BIND_MANAGED_FIELD(generation)>("generation");
	}
};

MAKE_TYPE_FACTORY(PulsePrefabHandle, PulsePrefabHandle);
struct PulsePrefabHandleAnnotation final : das::ManagedStructureAnnotation<PulsePrefabHandle>
{
	PulsePrefabHandleAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("PulsePrefabHandle", ml, "PulsePrefabHandle")
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
// Data table descriptor annotations (das builds schemas for
// pulse_data_table_system_register_schema; struct_size, version
// and fill_row are stamped by the binding, offsets and sizes are
// derived by the engine)
// ============================================================

DAS_BASE_BIND_ENUM(EPulseDataTableColumnType, EPulseDataTableColumnType,
	PULSE_DATA_TABLE_COLUMN_TYPE_INT,
	PULSE_DATA_TABLE_COLUMN_TYPE_FLOAT,
	PULSE_DATA_TABLE_COLUMN_TYPE_BOOL,
	PULSE_DATA_TABLE_COLUMN_TYPE_STRING,
	PULSE_DATA_TABLE_COLUMN_TYPE_ENUM,
	PULSE_DATA_TABLE_COLUMN_TYPE_STRUCT,
	PULSE_DATA_TABLE_COLUMN_TYPE_REF);

DAS_BIND_ENUM_CAST(EPulseDataTableColumnType);

MAKE_TYPE_FACTORY(PulseDataTableColumnDesc, PulseDataTableColumnDesc);
struct PulseDataTableColumnDescAnnotation final : das::ManagedStructureAnnotation<PulseDataTableColumnDesc>
{
	PulseDataTableColumnDescAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("PulseDataTableColumnDesc", ml, "PulseDataTableColumnDesc")
	{
		addField<DAS_BIND_MANAGED_FIELD(name)>("name");
		addField<DAS_BIND_MANAGED_FIELD(type)>("column_type");
		addField<DAS_BIND_MANAGED_FIELD(offset)>("offset");
		addField<DAS_BIND_MANAGED_FIELD(min_value)>("min_value");
		addField<DAS_BIND_MANAGED_FIELD(max_value)>("max_value");
		addField<DAS_BIND_MANAGED_FIELD(has_min)>("has_min");
		addField<DAS_BIND_MANAGED_FIELD(has_max)>("has_max");
		addField<DAS_BIND_MANAGED_FIELD(has_default)>("has_default");
		addField<DAS_BIND_MANAGED_FIELD(default_int)>("default_int");
		addField<DAS_BIND_MANAGED_FIELD(default_float)>("default_float");
		addField<DAS_BIND_MANAGED_FIELD(default_bool)>("default_bool");
		addField<DAS_BIND_MANAGED_FIELD(default_string)>("default_string");
		addField<DAS_BIND_MANAGED_FIELD(struct_type)>("struct_type");
		addField<DAS_BIND_MANAGED_FIELD(ref_type)>("ref_type");
		addField<DAS_BIND_MANAGED_FIELD(enum_type)>("enum_type");
	}
};

MAKE_TYPE_FACTORY(PulseDataTableStructDesc, PulseDataTableStructDesc);
struct PulseDataTableStructDescAnnotation final : das::ManagedStructureAnnotation<PulseDataTableStructDesc>
{
	PulseDataTableStructDescAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("PulseDataTableStructDesc", ml, "PulseDataTableStructDesc")
	{
		addField<DAS_BIND_MANAGED_FIELD(name)>("name");
		addField<DAS_BIND_MANAGED_FIELD(size)>("size");
		addField<DAS_BIND_MANAGED_FIELD(align)>("align");
		addFieldEx("p_columns", "p_columns", (off_t)offsetof(PulseDataTableStructDesc, p_columns), makeType<void*>(*mlib));
		addField<DAS_BIND_MANAGED_FIELD(columns_count)>("columns_count");
	}
};

MAKE_TYPE_FACTORY(PulseDataTableEnumDesc, PulseDataTableEnumDesc);
struct PulseDataTableEnumDescAnnotation final : das::ManagedStructureAnnotation<PulseDataTableEnumDesc>
{
	PulseDataTableEnumDescAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("PulseDataTableEnumDesc", ml, "PulseDataTableEnumDesc")
	{
		addField<DAS_BIND_MANAGED_FIELD(name)>("name");
		addFieldEx("p_values", "p_values", (off_t)offsetof(PulseDataTableEnumDesc, p_values), makeType<void*>(*mlib));
		addField<DAS_BIND_MANAGED_FIELD(values_count)>("values_count");
	}
};

MAKE_TYPE_FACTORY(PulseDataTableSchemaDesc, PulseDataTableSchemaDesc);
struct PulseDataTableSchemaDescAnnotation final : das::ManagedStructureAnnotation<PulseDataTableSchemaDesc>
{
	PulseDataTableSchemaDescAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("PulseDataTableSchemaDesc", ml, "PulseDataTableSchemaDesc")
	{
		addField<DAS_BIND_MANAGED_FIELD(name)>("name");
		addFieldEx("p_columns", "p_columns", (off_t)offsetof(PulseDataTableSchemaDesc, p_columns), makeType<void*>(*mlib));
		addField<DAS_BIND_MANAGED_FIELD(columns_count)>("columns_count");
		addFieldEx("p_structs", "p_structs", (off_t)offsetof(PulseDataTableSchemaDesc, p_structs), makeType<void*>(*mlib));
		addField<DAS_BIND_MANAGED_FIELD(structs_count)>("structs_count");
		addFieldEx("p_enums", "p_enums", (off_t)offsetof(PulseDataTableSchemaDesc, p_enums), makeType<void*>(*mlib));
		addField<DAS_BIND_MANAGED_FIELD(enums_count)>("enums_count");
		addField<DAS_BIND_MANAGED_FIELD(key_column)>("key_column");
		addField<DAS_BIND_MANAGED_FIELD(key_is_int)>("key_is_int");
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
DAS_PULSE_VALUE_CAST(PulseMaterialRequest);
DAS_PULSE_VALUE_CAST(PulseShaderHandle);
DAS_PULSE_VALUE_CAST(PulseMeshHandle);
DAS_PULSE_VALUE_CAST(PulseMaterialHandle);
DAS_PULSE_VALUE_CAST(PulsePrefabRequest);
DAS_PULSE_VALUE_CAST(PulsePrefabHandle);
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
		addAnnotation(new PulseMaterialRequestAnnotation(lib));
		addAnnotation(new PulseShaderHandleAnnotation(lib));
		addAnnotation(new PulseMeshHandleAnnotation(lib));
		addAnnotation(new PulseMaterialHandleAnnotation(lib));
		addAnnotation(new PulsePrefabRequestAnnotation(lib));
		addAnnotation(new PulsePrefabHandleAnnotation(lib));
		addAnnotation(new PulseAssetRequestAnnotation(lib));
		addAnnotation(new PulseMaterialCreateDescAnnotation(lib));
		addEnumeration(new EnumerationEPulseDataTableColumnType());
		addAnnotation(new PulseDataTableColumnDescAnnotation(lib));
		addAnnotation(new PulseDataTableStructDescAnnotation(lib));
		addAnnotation(new PulseDataTableEnumDescAnnotation(lib));
		addAnnotation(new PulseDataTableSchemaDescAnnotation(lib));

		addExtern<DAS_BIND_FUN(HMM_V3), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "HMM_V3", SideEffects::none, "HMM_V3")->args({ "x", "y", "z" });
		addExtern<DAS_BIND_FUN(HMM_V4), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "HMM_V4", SideEffects::none, "HMM_V4")->args({ "x", "y", "z", "w" });
		addExtern<DAS_BIND_FUN(HMM_TRS_bind)>(*this, lib, "HMM_TRS", SideEffects::modifyArgument, "HMM_TRS")->args({ "translation", "rotation", "scale", "out" });

		addExtern<DAS_BIND_FUN(das_get_app_from_world), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "pulse_get_app_from_world", SideEffects::modifyExternal, "pulse_get_app_from_world")->args({ "world" });

		addExtern<DAS_BIND_FUN(das_shader_request_to_asset_request), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "pulse_shader_request_to_asset_request", SideEffects::none, "pulse_shader_request_to_asset_request")->args({ "request" });
		addExtern<DAS_BIND_FUN(das_mesh_request_to_asset_request), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "pulse_mesh_request_to_asset_request", SideEffects::none, "pulse_mesh_request_to_asset_request")->args({ "request" });
		addExtern<DAS_BIND_FUN(das_material_request_to_asset_request), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "pulse_material_request_to_asset_request", SideEffects::none, "pulse_material_request_to_asset_request")->args({ "request" });
		addExtern<DAS_BIND_FUN(das_prefab_request_to_asset_request), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "pulse_prefab_request_to_asset_request", SideEffects::none, "pulse_prefab_request_to_asset_request")->args({ "request" });

		addExtern<DAS_BIND_FUN(das_get_asset_system), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "pulse_get_asset_system", SideEffects::modifyExternal, "pulse_get_asset_system")->args({ "app" });
		addExtern<DAS_BIND_FUN(das_asset_system_get_state)>(*this, lib, "pulse_asset_system_get_state", SideEffects::modifyExternal, "pulse_asset_system_get_state")->args({ "asset_system", "request" });
		addExtern<DAS_BIND_FUN(das_asset_system_get_error)>(*this, lib, "pulse_asset_system_get_error", SideEffects::modifyExternal, "pulse_asset_system_get_error")->args({ "asset_system", "request" });

		addExtern<DAS_BIND_FUN(das_load_shader), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "pulse_load_shader", SideEffects::worstDefault, "pulse_load_shader")->args({ "app", "path" });
		addExtern<DAS_BIND_FUN(das_load_mesh), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "pulse_load_mesh", SideEffects::worstDefault, "pulse_load_mesh")->args({ "app", "path" });
		addExtern<DAS_BIND_FUN(das_load_material), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "pulse_load_material", SideEffects::worstDefault, "pulse_load_material")->args({ "app", "path" });
		addExtern<DAS_BIND_FUN(das_shader_is_ready)>(*this, lib, "pulse_shader_is_ready", SideEffects::modifyExternal, "pulse_shader_is_ready")->args({ "app", "request" });
		addExtern<DAS_BIND_FUN(das_mesh_is_ready)>(*this, lib, "pulse_mesh_is_ready", SideEffects::modifyExternal, "pulse_mesh_is_ready")->args({ "app", "request" });
		addExtern<DAS_BIND_FUN(das_material_is_ready)>(*this, lib, "pulse_material_is_ready", SideEffects::modifyExternal, "pulse_material_is_ready")->args({ "app", "request" });
		addExtern<DAS_BIND_FUN(das_shader_get_handle), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "pulse_shader_get_handle", SideEffects::modifyExternal, "pulse_shader_get_handle")->args({ "app", "request" });
		addExtern<DAS_BIND_FUN(das_mesh_get_handle), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "pulse_mesh_get_handle", SideEffects::modifyExternal, "pulse_mesh_get_handle")->args({ "app", "request" });
		addExtern<DAS_BIND_FUN(das_material_get_handle), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "pulse_material_get_handle", SideEffects::modifyExternal, "pulse_material_get_handle")->args({ "app", "request" });
		addExtern<DAS_BIND_FUN(das_create_material), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "pulse_create_material", SideEffects::worstDefault, "pulse_create_material")->args({ "app", "desc" });
		addExtern<DAS_BIND_FUN(das_material_set_property_float4)>(*this, lib, "pulse_material_set_property_float4", SideEffects::modifyExternal, "pulse_material_set_property_float4")->args({ "app", "material", "name", "value" });

		addExtern<DAS_BIND_FUN(das_load_prefab), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "pulse_load_prefab", SideEffects::worstDefault, "pulse_load_prefab")->args({ "app", "path" });
		addExtern<DAS_BIND_FUN(das_prefab_is_ready)>(*this, lib, "pulse_prefab_is_ready", SideEffects::modifyExternal, "pulse_prefab_is_ready")->args({ "app", "request" });
		addExtern<DAS_BIND_FUN(das_prefab_get_handle), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "pulse_prefab_get_handle", SideEffects::modifyExternal, "pulse_prefab_get_handle")->args({ "app", "request" });
		addExtern<DAS_BIND_FUN(das_prefab_instantiate)>(*this, lib, "pulse_prefab_instantiate", SideEffects::worstDefault, "pulse_prefab_instantiate")->args({ "app", "prefab" });

		addExtern<DAS_BIND_FUN(das_data_table_register_schema)>(*this, lib, "pulse_data_table_register_schema", SideEffects::modifyExternal, "pulse_data_table_register_schema")->args({ "app", "desc", "context", "at" });
		addExtern<DAS_BIND_FUN(das_data_table_load), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "pulse_data_table_load", SideEffects::worstDefault, "pulse_data_table_load")->args({ "app", "schema", "path" });
		addExtern<DAS_BIND_FUN(das_data_table_is_ready)>(*this, lib, "pulse_data_table_is_ready", SideEffects::modifyExternal, "pulse_data_table_is_ready")->args({ "app", "request" });
		addExtern<DAS_BIND_FUN(das_data_table_get_error)>(*this, lib, "pulse_data_table_get_error", SideEffects::modifyExternal, "pulse_data_table_get_error")->args({ "app", "request", "context", "at" });
		addExtern<DAS_BIND_FUN(das_data_table_row_count)>(*this, lib, "pulse_data_table_row_count", SideEffects::modifyExternal, "pulse_data_table_row_count")->args({ "app", "schema" });
		addExtern<DAS_BIND_FUN(das_data_table_row_at)>(*this, lib, "pulse_data_table_row_at", SideEffects::modifyExternal, "pulse_data_table_row_at")->args({ "app", "schema", "index" });
		addExtern<DAS_BIND_FUN(das_data_table_find_row)>(*this, lib, "pulse_data_table_find_row", SideEffects::modifyExternal, "pulse_data_table_find_row")->args({ "app", "schema", "key" });
		addExtern<DAS_BIND_FUN(das_data_table_find_row_int)>(*this, lib, "pulse_data_table_find_row_int", SideEffects::modifyExternal, "pulse_data_table_find_row_int")->args({ "app", "schema", "key" });

		addExtern<DAS_BIND_FUN(das_data_table_read_int)>(*this, lib, "pulse_data_table_read_int", SideEffects::none, "pulse_data_table_read_int")->args({ "app", "schema", "row", "c0", "c1", "c2", "c3" });
		addExtern<DAS_BIND_FUN(das_data_table_read_float)>(*this, lib, "pulse_data_table_read_float", SideEffects::none, "pulse_data_table_read_float")->args({ "app", "schema", "row", "c0", "c1", "c2", "c3" });
		addExtern<DAS_BIND_FUN(das_data_table_read_bool)>(*this, lib, "pulse_data_table_read_bool", SideEffects::none, "pulse_data_table_read_bool")->args({ "app", "schema", "row", "c0", "c1", "c2", "c3" });
		addExtern<DAS_BIND_FUN(das_data_table_read_string)>(*this, lib, "pulse_data_table_read_string", SideEffects::none, "pulse_data_table_read_string")->args({ "app", "schema", "row", "c0", "c1", "c2", "c3", "context", "at" });
		addExtern<DAS_BIND_FUN(das_data_table_read_ref)>(*this, lib, "pulse_data_table_read_ref", SideEffects::none, "pulse_data_table_read_ref")->args({ "app", "schema", "row", "c0", "c1", "c2", "c3" });

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

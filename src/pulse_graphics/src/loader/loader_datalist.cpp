#include "../graphics_internal.h"

#include <cstring>

namespace pulse_graphics_internal {

int64_t enum_from_string(const PulseDatalist* node, const char* key, const NamedValue* table, size_t count, int64_t default_value) {
    const char* s = pulse_datalist_get_string(node, key, nullptr);
    if (s != nullptr) {
        for (size_t i = 0; i < count; ++i) {
            if (strcmp(s, table[i].name) == 0)
                return table[i].value;
        }
        return default_value;
    }
    return pulse_datalist_get_int(node, key, default_value);
}

EPulseShaderPropertyType prop_type_from_string(const PulseDatalist* prop) {
    static const NamedValue table[] = {
        { "unknown", PULSE_SHADER_PROPERTY_TYPE_UNKNOWN },
        { "float",   PULSE_SHADER_PROPERTY_TYPE_FLOAT },
        { "float2",  PULSE_SHADER_PROPERTY_TYPE_FLOAT2 },
        { "float3",  PULSE_SHADER_PROPERTY_TYPE_FLOAT3 },
        { "float4",  PULSE_SHADER_PROPERTY_TYPE_FLOAT4 },
        { "int",     PULSE_SHADER_PROPERTY_TYPE_INT },
        { "mat4",    PULSE_SHADER_PROPERTY_TYPE_MAT4 },
        { "texture", PULSE_SHADER_PROPERTY_TYPE_TEXTURE },
        { "sampler", PULSE_SHADER_PROPERTY_TYPE_SAMPLER },
    };
    return (EPulseShaderPropertyType)enum_from_string(prop, "type", table, sizeof(table) / sizeof(table[0]), PULSE_SHADER_PROPERTY_TYPE_UNKNOWN);
}

PulseDatalist* parse_datalist_bytes(const PulseAssetLoadTask* ctx, const char** out_error) {
    if (ctx->bytes_size == 0 || !ctx->p_bytes) {
        *out_error = "datalist loader: no data";
        return nullptr;
    }
    PulseDatalist* dl = pulse_datalist_create_from_text(static_cast<const char*>(ctx->p_bytes), ctx->bytes_size);
    if (!dl) {
        *out_error = pulse_datalist_last_error();
        return nullptr;
    }
    return dl;
}

} // namespace pulse_graphics_internal

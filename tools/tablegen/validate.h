#pragma once

#include "schema.h"

#include <unordered_map>

namespace tablegen {

struct StructLayout {
    std::string name{};
    uint32_t size = 0;
    uint32_t align = 1;
    std::unordered_map<std::string, uint32_t> offsets{};
};

struct LayoutSet {
    std::unordered_map<std::string, StructLayout> structs{};
    std::unordered_map<std::string, uint32_t> table_rows{};
    std::unordered_map<std::string, uint32_t> table_aligns{};
    std::unordered_map<std::string, uint32_t> table_keys{};
};

bool validate_library(const Library& library, std::vector<std::string>& errors);
bool is_scalar_type(std::string_view type);
bool compute_layouts(const Library& library, LayoutSet& out, std::vector<std::string>& errors);

} // namespace tablegen

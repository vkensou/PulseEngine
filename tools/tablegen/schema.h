#pragma once

#include "pulse_datalist.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace tablegen {

enum class Kind {
    Table,
    Struct,
    Enum,
};

struct Field {
    std::string name{};
    std::string type{};
    std::string ref{};
    std::vector<std::string> values{};
    bool is_key = false;
    bool has_default = false;
    bool has_min = false;
    bool has_max = false;
    double min_value = 0.0;
    double max_value = 0.0;
    int64_t default_int = 0;
    double default_float = 0.0;
    bool default_bool = false;
    std::string default_string{};
    std::string default_text{};
    int line = 0;
};

struct Schema {
    std::string name{};
    std::string path{};
    Kind kind = Kind::Struct;
    std::vector<Field> fields{};
    std::vector<std::string> values{};
    int key_index = -1;
};

struct Library {
    std::vector<Schema> schemas{};
    std::vector<std::string> errors{};

    const Schema* find(std::string_view name) const;
};

bool parse_schema_file(const std::string& path, Schema& out, std::vector<std::string>& errors);
bool load_library(const std::vector<std::string>& paths, Library& out);

} // namespace tablegen

#pragma once

#include "pulse_datatable.h"

#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>
#include <vector>

namespace pulse::datatable {

template <typename T>
using Bare = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename T>
struct ColumnType {
    static constexpr EPulseDataTableColumnType value = PULSE_DATA_TABLE_COLUMN_TYPE_ENUM;
};

template <>
struct ColumnType<int64_t> {
    static constexpr EPulseDataTableColumnType value = PULSE_DATA_TABLE_COLUMN_TYPE_INT;
};

template <>
struct ColumnType<double> {
    static constexpr EPulseDataTableColumnType value = PULSE_DATA_TABLE_COLUMN_TYPE_FLOAT;
};

template <>
struct ColumnType<bool> {
    static constexpr EPulseDataTableColumnType value = PULSE_DATA_TABLE_COLUMN_TYPE_BOOL;
};

template <>
struct ColumnType<std::string_view> {
    static constexpr EPulseDataTableColumnType value = PULSE_DATA_TABLE_COLUMN_TYPE_STRING;
};

template <typename T>
struct ColumnType<T*> {
    static constexpr EPulseDataTableColumnType value = PULSE_DATA_TABLE_COLUMN_TYPE_REF;
};

struct ColumnDescBuilder {
    PulseDataTableColumnDesc desc{};

    ColumnDescBuilder& name(std::string_view text) {
        desc.name = text.data();
        return *this;
    }

    ColumnDescBuilder& column_type(EPulseDataTableColumnType value) {
        desc.type = value;
        return *this;
    }

    ColumnDescBuilder& offset(uint32_t byte_offset) {
        desc.offset = byte_offset;
        return *this;
    }

    ColumnDescBuilder& min(double minimum) {
        desc.has_min = true;
        desc.min_value = minimum;
        return *this;
    }

    ColumnDescBuilder& max(double maximum) {
        desc.has_max = true;
        desc.max_value = maximum;
        return *this;
    }

    ColumnDescBuilder& default_int(int64_t value) {
        desc.has_default = true;
        desc.default_int = value;
        desc.default_float = static_cast<double>(value);
        return *this;
    }

    ColumnDescBuilder& default_float(double value) {
        desc.has_default = true;
        desc.default_float = value;
        desc.default_int = static_cast<int64_t>(value);
        return *this;
    }

    ColumnDescBuilder& default_bool(bool value) {
        desc.has_default = true;
        desc.default_bool = value;
        return *this;
    }

    ColumnDescBuilder& default_string(std::string_view value) {
        desc.has_default = true;
        desc.default_string = value.data();
        return *this;
    }

    ColumnDescBuilder& struct_name(std::string_view text) {
        desc.struct_type = text.data();
        return *this;
    }

    ColumnDescBuilder& ref_name(std::string_view text) {
        desc.ref_type = text.data();
        return *this;
    }

    ColumnDescBuilder& enum_name(std::string_view text) {
        desc.enum_type = text.data();
        return *this;
    }

    PulseDataTableColumnDesc build() const {
        return desc;
    }
};

struct StringVault {
    std::vector<char> bytes{};
    size_t used = 0;

    void reserve(size_t extra) {
        bytes.assign(used + extra + 64, '\0');
    }

    std::string_view append(std::string_view text) {
        size_t offset = used;
        if (offset + text.size() + 1 <= bytes.size()) {
            std::memcpy(bytes.data() + offset, text.data(), text.size());
            bytes[offset + text.size()] = '\0';
        } else {
            bytes.insert(bytes.end(), text.begin(), text.end());
            bytes.push_back('\0');
        }
        used = offset + text.size() + 1;
        return std::string_view(bytes.data() + offset, text.size());
    }
};

} // namespace pulse::datatable

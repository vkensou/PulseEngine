#pragma once

#ifndef PULSE_TABLES_GENERATED_H
#define PULSE_TABLES_GENERATED_H

#include "pulse_datatable.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pulse_tables
{

struct PulseSnakeConfigRow;

struct alignas(8) PulseSnakeConfigRow
{
    std::string_view id;
    double move_interval;
};
static_assert(sizeof(PulseSnakeConfigRow) == 24, "snake_config layout mismatch");

struct PulseSnakeConfigRowTable
{
    static PulseAssetRequest Load(PulseAppId app, const char* path = nullptr);
    static bool IsReady(PulseAppId app);
    static const char* GetError(PulseAppId app);
    static const PulseSnakeConfigRow* Rows(PulseAppId app, uint32_t& out_count);
    static const PulseSnakeConfigRow* GetRow(PulseAppId app, const char* key);
    static const char* DefaultPath();
};

EPulseResult RegisterSchemas(PulseDataTableSystemId system);

} // namespace pulse_tables

#endif // PULSE_TABLES_GENERATED_H

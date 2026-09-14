#pragma once

#ifndef PULSE_TABLES_GENERATED_H
#define PULSE_TABLES_GENERATED_H

#include "pulse_datatable.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pulse_tables
{

struct PulseDeclaresOtherRow;
struct PulseShell;
struct PulseInner;
struct PulseDeepRow;
struct PulseDupRow;
struct PulseEnmRow;
struct PulseExtraRow;
struct PulseItemRow;
struct PulseNumRow;
struct PulseRefRow;
struct PulseReqRow;
struct PulseRngRow;
struct PulseSkill;
struct PulseSnakeRow;
struct PulseTypRow;

struct alignas(8) PulseDeclaresOtherRow
{
    std::string_view id;
};
static_assert(sizeof(PulseDeclaresOtherRow) == 16, "declares_other layout mismatch");

struct alignas(8) PulseShell
{
    int64_t radius;
    std::string_view tag;
};
static_assert(sizeof(PulseShell) == 24, "shell layout mismatch");

struct alignas(8) PulseInner
{
    double power;
    PulseShell shell;
};
static_assert(sizeof(PulseInner) == 32, "inner layout mismatch");

struct alignas(8) PulseDeepRow
{
    std::string_view id;
    PulseInner inner;
};
static_assert(sizeof(PulseDeepRow) == 48, "deep layout mismatch");

struct alignas(8) PulseDupRow
{
    std::string_view id;
};
static_assert(sizeof(PulseDupRow) == 16, "dup layout mismatch");

struct alignas(8) PulseEnmRow
{
    std::string_view id;
    std::string_view kind;
};
static_assert(sizeof(PulseEnmRow) == 32, "enm layout mismatch");

struct alignas(8) PulseExtraRow
{
    std::string_view id;
};
static_assert(sizeof(PulseExtraRow) == 16, "extra layout mismatch");

struct alignas(8) PulseItemRow
{
    std::string_view id;
    std::string_view name;
    double weight;
};
static_assert(sizeof(PulseItemRow) == 40, "item layout mismatch");

struct alignas(8) PulseNumRow
{
    int64_t id;
    std::string_view label;
};
static_assert(sizeof(PulseNumRow) == 24, "num layout mismatch");

struct alignas(8) PulseRefRow
{
    std::string_view id;
    const PulseItemRow* link;
};
static_assert(sizeof(PulseRefRow) == 24, "ref layout mismatch");

struct alignas(8) PulseReqRow
{
    std::string_view id;
    int64_t required;
};
static_assert(sizeof(PulseReqRow) == 24, "req layout mismatch");

struct alignas(8) PulseRngRow
{
    std::string_view id;
    double value;
};
static_assert(sizeof(PulseRngRow) == 24, "rng layout mismatch");

struct alignas(8) PulseSkill
{
    double power;
    int64_t radius;
};
static_assert(sizeof(PulseSkill) == 16, "skill layout mismatch");

struct alignas(8) PulseSnakeRow
{
    std::string_view id;
    double interval;
    int64_t hp;
    bool big;
    const PulseItemRow* drop;
    PulseSkill skill;
    std::string_view price;
};
static_assert(sizeof(PulseSnakeRow) == 80, "snake layout mismatch");

struct alignas(8) PulseTypRow
{
    std::string_view id;
    int64_t count;
};
static_assert(sizeof(PulseTypRow) == 24, "typ layout mismatch");

struct PulseDeclaresOtherRowTable
{
    static PulseAssetRequest Load(PulseAppId app, const char* path = nullptr);
    static bool IsReady(PulseAppId app);
    static const char* GetError(PulseAppId app);
    static const PulseDeclaresOtherRow* Rows(PulseAppId app, uint32_t& out_count);
    static const PulseDeclaresOtherRow* GetRow(PulseAppId app, const char* key);
    static const char* DefaultPath();
};

struct PulseDeepRowTable
{
    static PulseAssetRequest Load(PulseAppId app, const char* path = nullptr);
    static bool IsReady(PulseAppId app);
    static const char* GetError(PulseAppId app);
    static const PulseDeepRow* Rows(PulseAppId app, uint32_t& out_count);
    static const PulseDeepRow* GetRow(PulseAppId app, const char* key);
    static const char* DefaultPath();
};

struct PulseDupRowTable
{
    static PulseAssetRequest Load(PulseAppId app, const char* path = nullptr);
    static bool IsReady(PulseAppId app);
    static const char* GetError(PulseAppId app);
    static const PulseDupRow* Rows(PulseAppId app, uint32_t& out_count);
    static const PulseDupRow* GetRow(PulseAppId app, const char* key);
    static const char* DefaultPath();
};

struct PulseEnmRowTable
{
    static PulseAssetRequest Load(PulseAppId app, const char* path = nullptr);
    static bool IsReady(PulseAppId app);
    static const char* GetError(PulseAppId app);
    static const PulseEnmRow* Rows(PulseAppId app, uint32_t& out_count);
    static const PulseEnmRow* GetRow(PulseAppId app, const char* key);
    static const char* DefaultPath();
};

struct PulseExtraRowTable
{
    static PulseAssetRequest Load(PulseAppId app, const char* path = nullptr);
    static bool IsReady(PulseAppId app);
    static const char* GetError(PulseAppId app);
    static const PulseExtraRow* Rows(PulseAppId app, uint32_t& out_count);
    static const PulseExtraRow* GetRow(PulseAppId app, const char* key);
    static const char* DefaultPath();
};

struct PulseItemRowTable
{
    static PulseAssetRequest Load(PulseAppId app, const char* path = nullptr);
    static bool IsReady(PulseAppId app);
    static const char* GetError(PulseAppId app);
    static const PulseItemRow* Rows(PulseAppId app, uint32_t& out_count);
    static const PulseItemRow* GetRow(PulseAppId app, const char* key);
    static const char* DefaultPath();
};

struct PulseNumRowTable
{
    static PulseAssetRequest Load(PulseAppId app, const char* path = nullptr);
    static bool IsReady(PulseAppId app);
    static const char* GetError(PulseAppId app);
    static const PulseNumRow* Rows(PulseAppId app, uint32_t& out_count);
    static const PulseNumRow* GetRow(PulseAppId app, int64_t key);
    static const char* DefaultPath();
};

struct PulseRefRowTable
{
    static PulseAssetRequest Load(PulseAppId app, const char* path = nullptr);
    static bool IsReady(PulseAppId app);
    static const char* GetError(PulseAppId app);
    static const PulseRefRow* Rows(PulseAppId app, uint32_t& out_count);
    static const PulseRefRow* GetRow(PulseAppId app, const char* key);
    static const char* DefaultPath();
};

struct PulseReqRowTable
{
    static PulseAssetRequest Load(PulseAppId app, const char* path = nullptr);
    static bool IsReady(PulseAppId app);
    static const char* GetError(PulseAppId app);
    static const PulseReqRow* Rows(PulseAppId app, uint32_t& out_count);
    static const PulseReqRow* GetRow(PulseAppId app, const char* key);
    static const char* DefaultPath();
};

struct PulseRngRowTable
{
    static PulseAssetRequest Load(PulseAppId app, const char* path = nullptr);
    static bool IsReady(PulseAppId app);
    static const char* GetError(PulseAppId app);
    static const PulseRngRow* Rows(PulseAppId app, uint32_t& out_count);
    static const PulseRngRow* GetRow(PulseAppId app, const char* key);
    static const char* DefaultPath();
};

struct PulseSnakeRowTable
{
    static PulseAssetRequest Load(PulseAppId app, const char* path = nullptr);
    static bool IsReady(PulseAppId app);
    static const char* GetError(PulseAppId app);
    static const PulseSnakeRow* Rows(PulseAppId app, uint32_t& out_count);
    static const PulseSnakeRow* GetRow(PulseAppId app, const char* key);
    static const char* DefaultPath();
};

struct PulseTypRowTable
{
    static PulseAssetRequest Load(PulseAppId app, const char* path = nullptr);
    static bool IsReady(PulseAppId app);
    static const char* GetError(PulseAppId app);
    static const PulseTypRow* Rows(PulseAppId app, uint32_t& out_count);
    static const PulseTypRow* GetRow(PulseAppId app, const char* key);
    static const char* DefaultPath();
};

EPulseResult RegisterSchemas(PulseDataTableSystemId system);

} // namespace pulse_tables

#endif // PULSE_TABLES_GENERATED_H

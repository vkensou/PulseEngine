// ============================================================
// daslang 测试注入辅助：手动模式（不经 package_loader）下，通过
// pulse_package_get_runtimes 公开协议拿到 daslang runtime 的 load 回调，
// 枚举 VFS 目录树、读出文本、逐个注入——复刻 loader 阶段二的最小实现，
// 用于验证 daslang 对外注入通道（G4.1）在非 loader 场景可用。
// ============================================================
#pragma once

#include <assert.h>
#include <stdio.h>
#include <string>
#include <vector>

#include "pulse_app.h"
#include "pulse_script_register.h"
#include "pulse_vfs.h"

// pulse_package_get_runtimes 是 pulse_daslang.dll 的导出符号（协议入口），
// 不在 IDL 生成的 pulse_daslang.h 中；测试手动声明以便调用。
extern "C" uint32_t pulse_package_get_runtimes(const PulseScriptRuntimeDesc** out_runtimes);

namespace daslang_inject_helper {

struct CollectCtx {
    std::vector<std::string> files;
};

inline EPulseVfsEnumerateResult collect_callback(void* data, const char* orig_dir, const char* fname) {
    auto* ctx = static_cast<CollectCtx*>(data);
    if (!fname || !fname[0] || std::strcmp(fname, ".") == 0 || std::strcmp(fname, "..") == 0)
        return PULSE_VFS_ENUMERATE_RESULT_OK;

    std::string full;
    if (orig_dir && orig_dir[0]) {
        full = orig_dir;
        if (full.back() != '/' && full.back() != '\\') full += '/';
        full += fname;
    } else {
        full = fname;
    }

    PulseVfsStat st{};
    if (!pulse_vfs_stat(full.c_str(), &st))
        return PULSE_VFS_ENUMERATE_RESULT_OK;

    if (st.file_type == PULSE_VFS_FILE_TYPE_DIRECTORY) {
        CollectCtx sub{};
        pulse_vfs_enumerate(full.c_str(), collect_callback, &sub);
        for (auto& p : sub.files) ctx->files.push_back(std::move(p));
    } else if (st.file_type == PULSE_VFS_FILE_TYPE_REGULAR) {
        size_t len = full.size();
        if (len >= 4 && full.compare(len - 4, 4, ".das") == 0)
            ctx->files.push_back(std::move(full));
    }
    return PULSE_VFS_ENUMERATE_RESULT_OK;
}

inline const char* runtime_type_of(const PulseScriptRuntimeDesc* runtimes, uint32_t count) {
    (void)runtimes;
    (void)count;
    return "daslang";
}

// 枚举 VFS 根目录树下所有 .das，通过 daslang 的 load 回调注入。
// 返回注入的文件数量。
inline int inject_all_das(PulseAppId app) {
    const PulseScriptRuntimeDesc* runtimes = nullptr;
    uint32_t count = pulse_package_get_runtimes(&runtimes);
    assert(count > 0 && runtimes && runtimes[0].load);

    CollectCtx ctx{};
    pulse_vfs_enumerate("", collect_callback, &ctx);

    int injected = 0;
    for (const std::string& path : ctx.files) {
        PulseVfsFileId file = pulse_vfs_open_read(path.c_str());
        assert(file);
        int64_t length = pulse_vfs_file_length(file);
        assert(length >= 0);
        std::string text;
        text.resize(static_cast<size_t>(length));
        if (length > 0) {
            int64_t got = pulse_vfs_read_bytes(file, text.data(), static_cast<uint64_t>(length));
            assert(got == length);
        }
        pulse_vfs_close(file);
        assert(runtimes[0].load(app, path.c_str(), text.data(), static_cast<uint64_t>(text.size())) == PULSE_RESULT_OK);
        ++injected;
    }
    return injected;
}

} // namespace daslang_inject_helper
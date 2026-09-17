# pulse_font 改造计划（bitmap 字体 + asset 接入 + 默认字体 + atlas/渲染隔离）

本文是对 [文字渲染设计.md](文字渲染设计.md) 已落地实现的四项改造。执行者假定不了解前情，本文自包含。动手前先读 `AGENTS.md`，全程遵守其中规则，特别是：

- `src/pulse_font/include/pulse_font.h` 与 `src/pulse_font/src/package_register.cpp` 禁止手改，只能改 `src/pulse_font/idl/pulse_font.idl` 后用 `tools/idl` 与生成工具重新生成。
- 禁止编写注释；同一条语句禁止换行；不做兼容，不留旧接口。
- 构建逐个 target 执行 `xmake build pulse_font`；测试用 `xmake test`；带窗口验证用 skill `test-tool-for-program-with-window`。

现状代码结构（改造对象）：

- `src/pulse_font/src/font_core.cpp`：插件生命周期、字体注册表、chain、度量、公开 API 实现。
- `src/pulse_font/src/font_atlas.cpp`：SDF 图集（页管理、哈希、LRU、EDT 光栅化、CPU 像素）。
- `src/pulse_font/src/font_render.cpp`：GPU 侧（图集纹理创建/上传、shader/material/mesh/sampler、rendergraph record callback、`pulse_font_submit`）。
- `src/pulse_font/src/font_glyph.vs.hlsl` / `font_glyph.ps.hlsl`：SDF instanced quad shader。
- `tests/font/`：`test_registry.cpp`、`test_sdf.cpp`、`test_atlas.cpp`（纯 CPU）、`font_window.cpp`（带窗口）、测试数据 `data/latin.ttf`、`data/cjk.ttf`。

实施顺序固定为：**阶段一（隔离重构）→ 阶段二（bitmap 字体）→ 阶段三（asset 接入）→ 阶段四（内置默认字体）**。每个阶段独立提交、独立测试全绿后再进下一阶段。

---

## 阶段一：atlas 更新与文字渲染隔离

目标：pulse_font 内部形成"CPU 图集模块"与"GPU 渲染模块"的单向依赖（render 依赖 atlas，atlas 不知道 render 存在），并把渲染模块将来整体搬出 pulse_font 所需的 capi 补齐。本阶段不改任何公开 API 语义，只新增。

### 改动

1. 删除反向耦合：`font_atlas.cpp` 中对 `render_ensure_page` 的调用（当前在创建新页处）删除。改为 `font_render.cpp` 的 `font_record_callback` 在帧回调开头遍历 `state->pages`，对尚无 GPU 纹理的页调 `render_ensure_page` 补建。`font_internal.h` 中 `render_ensure_page` 声明保留（仍是 render 模块内部函数）。
2. dirty 标志换成版本号：`atlas_page.dirty` 改为 `uint64_t version`，页内容每次变更（光栅化写入、页创建）时 `++version`；`font_page_gpu` 记录 `last_uploaded_version`，record callback 里 `version != last_uploaded_version` 才走上传 pass，上传后同步。GPU 纹理首次 ready 时 `last_uploaded_version` 置 0 强制首传。
3. 新增图集访问 capi（IDL 同步新增，供将来渲染包跨 dll 使用）：
   - `uint32_t pulse_font_page_count(PulseAppId app)`：当前图集页数。
   - `Pulse_Blob pulse_font_page_pixels(PulseAppId app, uint32_t page)`：返回该页 CPU 像素（R8，尺寸 = plugin desc 的 atlas_width × atlas_height），页不存在返回空 blob。实现直接引用内部 `pixels` 数据，不拷贝。
   - `uint64_t pulse_font_page_version(PulseAppId app, uint32_t page)`：返回该页当前版本号。
   - 页宽高不新增接口，沿用 `PulseFontPluginDesc.atlas_width/atlas_height`。
4. 物理隔离：`font_render.cpp`、两个 hlsl、spv 头文件生成规则挪进 `src/pulse_font/src/render/` 子目录（xmake.lua 的源文件通配确认覆盖子目录）；`font_atlas.cpp` 不得 include 任何 `pulse_graphics.h` / render 侧头文件，`font_internal.h` 中 atlas 与 render 的函数声明分成两个明确区块。
5. `pulse_font_submit`、`PulseDrawDesc`、`PulseGlyphInstance`、`PulseTransform`、`PulseScissor`、`record_priority` 本阶段原地不动，但它们属于"将来搬走"的集合，IDL 里集中放到文件尾部一个区块，块前加文档注释说明该区块整体属于渲染出口（IDL 文件允许注释，生成的头文件注释由工具产出）。

### 验收

- `xmake test` 全绿（`tests/font` 三个 CPU 测试不改语义；若测试直接摸了 `dirty` 字段则同步改）。
- `font_window` 带窗口验证：中英文混排渲染结果与改造前一致（截图比对）。
- grep 确认 `font_atlas.cpp` 无 `render_` 前缀调用、无 pulse_graphics 依赖。

---

## 阶段二：bitmap 字体支持

目标：在现有 TTF（stb_truetype）之外支持 AngelCode BMFont 文本格式（`.fnt` + 单页 PNG）。核心设计决策：**bitmap 字形不新增渲染路径**，字形位图作为 mask 喂进现有 EDT → SDF 管线，图集、shader、`PulseGlyph`、submit 全部不变。

### 字体源抽象

`font_face` 改为带类型标记的结构：

- `enum font_kind { kFontKindTtf, kFontKindBitmap }`。
- TTF 分支保持现状（`stbtt_fontinfo` + data）。
- bitmap 分支持有：解析后的 BMFont 数据（每字符的 `x, y, width, height, xoffset, yoffset, xadvance, page`、kerning 对表、`lineHeight / base / scaleW / scaleH`）、解码后的 PNG 像素（R8 或 RGBA 取 alpha/灰度）、family 名（取 `.fnt` 的 `face` 字段）。
- 现有 `font_scale_for_size / font_glyph_index / font_advance_raw / font_kerning_raw` 改为按 kind 分派：
  - bitmap 的 `glyph_index` 语义 = "该 codepoint 是否有字形"（有返回非 0）。
  - bitmap 的 `advance_raw` = `xadvance`，**不随 size 缩放**（见下"档位"）。
  - bitmap 的 kerning 查 `.fnt` kerning 表，无则 0。
  - bitmap 的垂直度量：ascent = `base`，descent = `lineHeight - base`，line_gap = 0。
- 光栅化路径分派：TTF 走 stbtt 光栅化到 mask；bitmap 直接从 PNG 像素拷贝字形矩形到 mask（含 padding 对位）。之后统一走现有 EDT → 槽位写入，`glyph_entry` 的 `x0/y0/x1/y1`（相对笔尖基线的摆放矩形）由 `xoffset/yoffset/width/height` 与 `base` 换算，与 TTF 路径产出同一坐标系（y 向下、原点笔尖基线）。

### 尺寸档位（tier）处理

bitmap 字体只有一个原生像素尺寸，不参与 `{48, 96}` 双档位光栅化：

- bitmap face 记录原生高度 `native_size`（= `.fnt` 的 lineHeight 或字符高度，取解析时确定的单一值）。
- 请求字号 ≤ native_size × 1.5 时落 tier 0，按 native_size 光栅化一次；更大字号落 tier 1，同样只按 native_size 光栅化（槽位内 mask 先按最近邻放大到目标档位基准尺寸再 EDT，避免 SDF 距离分辨率不足）。`make_glyph` 中 `scale = size / kTierBaseSizes[tier]` 的现有逻辑对 bitmap 不适用，改为 bitmap 分支按 `size / native_size` 直接缩放摆放矩形与 uv 外的几何量；具体换算以"渲染出的字形像素高度 ≈ 请求 size"为准，用测试锁定。
- `glyph_key` 不变（font, codepoint, tier），tier 对 bitmap 只有档位选择意义。

### 格式与依赖

- 只支持 BMFont **文本格式** `.fnt`（`info/common/page/chars/char/kerning` 行），单页（`pages=1`），PNG 纹理与 `.fnt` 同目录、文件名取 `page` 行的 `file` 字段。多页、二进制 `.fnt` 不支持，解析到即报错。
- vendor `stb_image.h` 到 `src/pulse_font/src/`（从 `src/pulse_graphics/src/loader/stb_image.h` 拷贝，实现宏放在 bitmap 解析的单一 cpp 里）。
- PNG 解码后统一转成 R8 灰度 mask（RGBA 取 alpha 通道，灰度图直接用）。
- 解析入口沿用现有 `pulse_font_register(memory)` 无法表达"两个文件"，因此本阶段新增内存注册接口：`pulse_font_register_bitmap(PulseAppId app, Pulse_Blob_Param(fnt), Pulse_Blob_Param(png))`，返回字体 id；TTF 仍走 `pulse_font_register`。`register` 内部按字节 magic 判断 kind（`.fnt` 文本以 `info ` 开头），TTF 路径误传 fnt 时报错。（注：本接口在阶段三会被 asset loader 取代，属于过渡，阶段三完成后删除。）

### 测试

- `tests/font/data/` 新增一套小尺寸 ASCII BMFont（可用 BMFont 工具或脚本从现有 latin.ttf 生成，16px 与 32px 各一套更好，非必须）。
- `tests/font/test_bitmap.cpp`（纯 CPU）：解析正确性（字符数、lineHeight、抽样 char 字段）；`advance` 不随 size 变化的部分与随 size 缩放的部分符合预期；`pulse_font_glyph` 返回 valid 且 uv 落在正确槽位；`pulse_font_atlas_sample` 在字形矩形内采到 > cutoff 的值、在槽位 padding 外采到 < cutoff 的值（验证 mask 确实进了 EDT）；缺字符回退 missing glyph 行为与 TTF 一致。
- 现有测试全部保持绿。

### 验收

- `xmake test` 全绿。
- `font_window` 增加一行 bitmap 字体渲染文本，截图确认字形可辨、边缘平滑（SDF 生效）。

---

## 阶段三：字体加载走 pulse_asset

目标：删除同步注册 API，字体成为 asset 类型，加载/生命周期由 pulse_asset 统一管理。范本照抄 pulse_graphics 的 texture：`src/pulse_graphics/src/assets/texture.cpp`（类型注册）+ `src/pulse_graphics/src/loader/load_texture.cpp`（loader 注册与实现）+ `tests/asset/` 的加载测试写法。

### 类型与 loader

- `PULSE_TYPE_FONT` 定义为 `UINT64_C(0x4000)`，写进 IDL 生成的 `pulse_font.h`（现有已占用段：graphics 0x1000、daslang/prefab 0x2000、datatable 0x3000）。
- 资产数据结构 `PulseFontAssetData`（插件内部，位于 out_asset 内存）：parsed 字体（kind、family、TTF 原始字节 + stbtt_fontinfo 重建所需 offset，或 bitmap 解析结果 + PNG 像素）、`face_index`。destroy 回调释放内部堆内存。
- loader 注册两个：
  - extensions `"ttf,otf,ttc"`：读文件字节 → 按 `face_index`（settings 传入，默认 0）初始化 stbtt → 填 asset data。TTC 的 face 数量校验失败则 loader step 返回 FAILED 并给出 error 字符串。
  - extensions `"fnt"`：读 `.fnt` 字节 + 同目录 PNG（经 VFS）→ 解析 → 填 asset data。
- settings 结构 `PulseFontLoadSettings { uint32_t face_index; }`，经 `PulseAssetLoadDesc.settings` 传入。
- 注册时机：`pulse_font_plugin_post_build` 中检测到 `pulse_get_asset_system(app)` 存在时注册类型与 loader（与 graphics 插件同款守卫）；`PulsePluginDesc.dependencies` 增加 `"pulse_asset"`；`package.json` 的 dependencies 增加 `"pulse_asset"`（置于 pulse_graphics 之前）。

### 公开 API 变更（IDL）

删除：`FontRegister`、`FontRegisterFile`、`FontRegisterBitmap`（阶段二过渡接口）、`FontFaceCount`（face 数量校验移入 loader；调用方传错 face_index 会得到加载失败与错误信息）。

新增（Request/Handle 值类型与转换 helper 照抄 pulse_graphics 的 texture 模式，由 IDL 生成）：

- `PulseFontRequest pulse_font_load(PulseAppId app, const char* path, uint32_t face_index)`：发起异步加载，内部 `pulse_asset_system_load`。
- `PulseFontRequest pulse_font_load_from_memory(PulseAppId app, const char* name, Pulse_Blob_Param(memory), uint32_t face_index)`：内部 `pulse_asset_system_load_from_memory`，`name` 用于扩展名判定（如 `"latin.ttf"`、`"ascii.fnt"`）。
- `bool pulse_font_is_ready(PulseAppId app, PulseFontRequest request)` / `bool pulse_font_is_alive(PulseAppId app, PulseFontRequest request)` / `const char* pulse_font_get_error(PulseAppId app, PulseFontRequest request)`：转发 asset system。
- `uint32_t pulse_font_acquire(PulseAppId app, PulseFontRequest request)`：asset ready 后调用；borrow 资产数据，在插件注册表登记（分配字体 id，持有资产 retain），返回字体 id；未 ready 或重复 acquire 返回 `PULSE_FONT_ID_NONE`（同一路径 + face_index 重复加载时 asset system 命中缓存，注册表按 asset handle 去重返回已有 id）。
- `void pulse_font_release_font(PulseAppId app, uint32_t font)`：释放资产（asset system release）并回收 id。id 被 chain 引用时先要求销毁 chain，注册表不做级联（与现有 chain 生命周期一致）。
- 保留：`FontCount`、`FontFamilyName`、`FontFindFamily`、chain 全部接口、度量全部接口、`FontGlyph`、`FontPrewarm`、atlas 全部接口、submit。语义不变。

注册表内部调整：`state->fonts` 元素增加 `PulseAssetHandle` 字段；`family` 取自 asset data。字体 id 仍从 1 起分配、支持复用（free list，与 chain 同法）。

### 测试迁移

- `tests/font/test_common.h` 的 app 装配增加 `pulse_add_asset_plugin`；所有 `pulse_font_register*` 调用改为 `load_from_memory`（TTF 字节沿用现有 data 文件读入）+ 泵 asset system 至 ready + `acquire`，参考 `tests/asset/test_basic.cpp` 的驱动方式；teardown 增加 `release_font`。
- 新增 `test_asset_load.cpp`：文件路径加载 ttf 与 fnt 各一次；错误 face_index 加载失败且 `get_error` 非空；同路径二次加载命中缓存（acquire 得到同一 id）；release 后 `font_count` 回落。
- `font_window.cpp` 同步迁移。

### 验收

- `xmake test` 全绿；`font_window` 截图与阶段二一致。
- 无 asset 插件时（纯 CPU 只装 vfs 的进程）`pulse_font_load` 返回无效 request 且不崩溃；度量/glyph API 对已 acquire 的 id 照常工作（asset 系统本身纯 CPU，不依赖 graphics）。

---

## 阶段四：内置默认 bitmap 字体

目标：插件自带一个 ASCII（codepoint 32–126）bitmap 字体，无外部文件即可用，作为所有 chain 的兜底。

### 资产内嵌

- 字体来源：维护者（人类）下载/生成 BMFont 文本格式 `.fnt` + PNG，放到 `src/pulse_font/assets/default_font/`（`default.fnt` + `default.png`，建议 16px 或 24px 行高、含 32–126 全部字符）。许可证要求：可再分发（如源自公有领域或 OFL 字体的 BMFont 导出），在目录内放 `LICENSE.txt` 记录来源。
- 新增生成脚本 `tools/gen_default_font/`（PowerShell 或 python，与仓库现有工具风格一致）：把两个文件转成 `src/pulse_font/src/default_font_data.h`（两个 `unsigned char` 数组 + 长度宏）。生成物提交进仓库；xmake 不做构建期生成。
- 加载方式：`post_build` 中（asset 系统存在时）走 `pulse_asset_system_build_sync`，builder loader identifier `"pulse_font_default"`，ctor/step 从内嵌字节构造 asset data（复用阶段三 fnt loader 的解析函数）。同步构建保证 post_build 返回后默认字体即可用。

### 公开 API 与 chain 行为

- 新增 `uint32_t pulse_font_default(PulseAppId app)`：返回默认字体的注册表 id（post_build 时已 acquire）。asset 系统缺失时返回 `PULSE_FONT_ID_NONE`。
- chain 兜底：`pulse_font_create_chain` 在用户给的字体列表末尾自动追加默认字体 id（已存在则不重复追加）。效果：任何 chain 对 ASCII 永不缺字；CJK 缺字行为不变（默认字体没有 CJK，仍落 missing glyph 方块）。`pulse_font_resolve_codepoint` / 度量的 fallback 语义不变。
- 默认字体不计入用户可见的 `pulse_font_count`？—— 计入（保持"count = 注册表大小"的简单语义），文档注释说明 id 可能由插件内建产生。

### 测试

- `test_registry.cpp` 或新文件：`pulse_font_default` 返回有效 id；对 `'A'`、`'z'`、`'0'` 的 `glyph` 均 valid；只含一个 CJK 字体（cjk.ttf）的 chain 对 `'A'` resolve 到默认字体 id；对 CJK codepoint 的 resolve 不受默认字体影响。
- `font_window`：不注册任何用户字体，直接用 `create_chain({default})` 渲染一行英文，截图验收。

### 验收

- `xmake test` 全绿；`font_window` 截图确认默认字体渲染清晰。
- 全新 clone + 不放置任何字体文件的情况下，阶段四测试全部通过（验证"内嵌"成立）。

---

## 开放问题（动手前需维护者确认）

1. **默认字体素材**：维护者计划下载哪套 bitmap 字体？格式必须是 BMFont 文本 `.fnt` + 单页 PNG；若拿到的是其他格式（二进制 fnt、多图页、纯 PNG 网格），先反馈，不要自行扩展解析器。
2. **`PULSE_TYPE_FONT = 0x4000`**：如引擎另有 type_id 分配约定，以约定为准。
3. **chain 自动追加默认字体**：本计划采用"自动追加"，若希望显式可控（调用方自己把 default 放进 chain），在阶段四开工前提出，改动很小。
4. **bitmap 字体是否需要支持彩色（RGBA 彩色字形）**：本计划按灰度/alpha 处理，彩色 PNG 只取 alpha。如需彩色 emoji 类字形，超出本次范围。

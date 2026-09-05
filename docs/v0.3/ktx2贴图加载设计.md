# KTX2 贴图加载完整支持方案（UASTC / ETC1S / 原生块格式 + 不支持时的回退）

对应 `docs/v0.3/版本任务.md` 第 3 项「贴图使用 ktx 处理」。工具侧说明见 `tools/ktx/README.md`。

## 1. 结论先说

1. 引擎读不了压缩贴图，根因是三层都缺东西，不是单点问题：**libktx 没编转码器** → **loader 没有 vkFormat→CGPU 表且按 texel 搬数据** → **cgpu 的 Vulkan 格式翻译表里根本没有 BC/ETC2/ASTC**。三层必须一起补，缺任何一层都跑不通。
2. UASTC 和 ETC1S 都走同一条路：`ktxTexture2_TranscodeBasis()` 在内存里把文件转成设备原生块格式再上传。ETC1S 需要先解 BasisLZ 超压缩、UASTC 可能带 zstd，这些 libktx 内部都处理了，loader 只管调一次函数。
3. 回退能力取决于文件类型，不取决于引擎聪明程度：
   - **可转码文件（UASTC / ETC1S）有真回退链**：BC7 → BC1/BC3 → (BC5/BC4，仅线性，本期未进链) → ASTC 4x4 → ETC2 → RGBA32。逐级用设备能力过滤，RGBA32 是无条件兜底。
   - **原生块格式文件（BC / ASTC / ETC2）没有回退**：运行时没有 BC/ASTC 解码器（`src/khr/ktx/lib` 里只有 UASTC 解码器和 `astc_encode.cpp` 编码器，`etcdec.cxx` 没有参与编译），设备不支持就只能报错。提示语要正确：只能**从 UASTC/ETC1S 母文件重新 `ktx transcode` 出目标格式**——`transcode` 读不了已块压缩的文件（§5.2）。
   - **原生未压缩格式按逐个情况看**：`R8G8B8` 不是 Vulkan 强制格式，个别驱动缺 SAMPLE 位，但它与 `R8G8B8A8` 逐 texel 语义严格等价（1×1 块、无行填充），所以支持 CPU 展开回退（§5.3）；其它未压缩格式没有等价替身，仍然报错。
   - 所以约定：**UASTC/ETC1S 是母格式（可跨平台转码），BC（桌面）/ASTC（移动）是发布产物**。
4. cgpu 是 submodule（`github.com/vkensou/cgpu`），必须改、必须单独提交，父仓同步 gitlink。无法在 loader 侧绕过：格式枚举→VkFormat 是后端职责。
5. 已实测的关键前提：把 `basis_transcode.cpp` + `basisu_transcoder.cpp` 加进 `ktx` 目标，在 MSVC / cxx20 / `set_exceptions("none")` 下 **2.25 秒编过，链接通过**（见 §6 步骤 1，改动已在仓库里）。

## 2. 现状与阻塞点

### 2.1 loader：`src/pulse_graphics/src/loader/load_texture.cpp`

| 位置 | 问题 |
| --- | --- |
| `:238` | 只注册扩展名 `ktx`。`ktx create` 永远输出 KTX2 容器（`tools/ktx/README.md` §"能不能只用 ktx.exe"），`.ktx2` 后缀根本没有 loader。 |
| `:75-104` | `detectKtxTextureFormat` 只认 vkFormat `23`/`37`。`23` 实际是 `VK_FORMAT_R8G8B8_UNORM`（`src/khr/ktx/lib/vkformat_enum.h:45`），却被映射成 `CGPU_TEXTURE_FORMAT_R8G8B8A8_SRGB`：线性数据被当 sRGB，和 `--assign-oetf linear` 的产物语义相反。`43 = VK_FORMAT_R8G8B8A8_SRGB` 不在表里，最常见的产物直接失败。 |
| `:77-90` | KTX1 的 `glInternalformat` 分支只认 3 个值，`toktx` 默认写的 `0x8C43`（GL_SRGB8_ALPHA8）不在表里。仓库里唯一的 `.ktx` 资产 `tests/graphics/data/TilesGray512.ktx` 实测头是 `«KTX 20»` + `vkFormat: VK_FORMAT_R8G8B8_UNORM`，**本来就是 KTX2**，KTX1 分支可以整段删掉。 |
| `:128` | `isCompressed` 直接失败 → BC/ASTC/UASTC/ETC1S 全部读不了。 |
| `:173`、`:185-202` | 拷贝按 `mipW*mipH*component` 逐 texel 搬，并用 `textureComponent != component` 走 3→4 展开分支。块格式必须按块搬（大小 = 向上取整的块数 × 块字节数），这套双轨逻辑在块格式下不成立。`FormatUtil_BitSizeOfBlock(CGPU_TEXTURE_FORMAT_R8G8B8_SRGB)` 已经返回 24（`cgpu/src/cgpu/include/cgpu/api.h:2482`），RGB8 可以直接 3 字节上传，**无条件**展开分支没有必要；设备缺 SAMPLE 位时的按需展开见 §5.3。 |
| `:137`、`:146-150`、`:188` | `arraySize` 只区分 1 / cubemap(6)，但拷贝循环遍历 `numLayers * numFaces`。纹理数组（`numLayers > 1`）会往只有一层的 staging 里多写 → 越界。 |
| `:139-145` | `generate_mipmaps` 靠额外申请 mip + `CGPU_RESOURCE_TYPE_RENDER_TARGET` + 运行时生成。压缩格式不能当 RT，也不该运行时生成（`tools/ktx/README.md` §5），这条路对压缩贴图必须关闭。 |
| `create_texture.cpp:8` | 顺带：`TextureLoaderState` 在 `load_texture.cpp:10` 和 `create_texture.cpp:8` 各定义一次（同命名空间同名的不同类，ODR 违规，恰好布局相同没炸）。本期顺手合并进 `graphics_internal.h`。 |

### 2.2 libktx：`src/khr/xmake.lua`

`ktx` 目标只编了 `texture*.c / memstream / filestream / zstd / miniz_wrapper / vkformat_*`，**没有** `ktx/lib/basis_transcode.cpp`（`ktxTexture2_TranscodeBasis` 的实现所在）和 `ktx/lib/basisu/transcoder/basisu_transcoder.cpp`（Basis 解码/转码器 + 表）。`ktxTexture2_NeedsTranscoding` 在 `texture2.c:1929` 里已有，能编译但没人能转码。源码是齐的，不需要下载任何东西。

另外两个编译细节：

- `basis_transcode.cpp:25` 写的是 `#include "dfdutils/dfd.h"`，而 `ktx` 目标的 include 路径只有 `src/khr/dfdutils`，需要把 `src/khr` 也加进去。
- `basisu/transcoder/basisu_transcoder.h:3` 明确要求 gcc/clang 关闭严格别名（`-fno-strict-aliasing`），Android 构建必须加，MSVC 无此选项。

### 2.3 cgpu：Vulkan 后端格式表是空的

- `cgpu/src/cgpu/backend/vulkan/include/cgpu_vulkan.inl:17-124` 的 `VkUtil_FormatTranslateToVk` **一个块压缩分支都没有**，所有 BC/ETC2/EAC/ASTC 走 `default: return VK_FORMAT_UNDEFINED`。后果：
  - `cgpu_vulkan_resources.c:767` 建 image 时 `VkImageCreateInfo.format = VK_FORMAT_UNDEFINED` → 创建失败/验证层报错。
  - `cgpu_vulkan_resources.c:1425` 建 texture view 同理。
  - `vulkan_utils.c:965-1004` 的 `VkUtil_CheckFormatSupport` 开头 `if (fmt == VK_FORMAT_UNDEFINED) return;`，于是 `adapter_detail.format_supports[BC7]` 永远是 0 → **我们的回退判定会误判"设备不支持 BC7"**，永远掉到 RGBA32。也就是说这张表不补，回退链本身就是坏的。
  - 反向的 `VkUtil_FormatTranslateToCGPU`（`:161-308`）同样把块格式判成 UNDEFINED（`:305`）。
- 注意 CGPU 枚举序号 ≠ VkFormat 序号（BC1：CGPU 105 / VK 131；ETC2 RGBA：133 / 151；ASTC 4x4：139 / 157），且 VK 的 `*_USCALED/SSCALED` 与 `A?B?G?R?_PACK32` 群在 CGPU 里不存在，所以只能显式查表，不能加减偏移。
- 块数取整不一致，非块整数倍尺寸会踩：`cgpu_vulkan_resources.c:315-321` 用 `width / WidthOfBlock`（向下取整）算 `bufferRowLength`，而 `src/pulse_graphics/src/runtime/upload.cpp:44-46` 与 `rendergraph/src/rendergraph.cpp:564-567` 用向上取整算大小。640×480 + ASTC 6x6 时 107 块 vs 106 块：debug 下 `rendergraph.cpp:568` 的 `assert(bufferSize >= size + offset)` 直接炸，release 下 `rendergraph_executor.cpp:336` 的 memcpy 越界；即便侥幸通过，`bufferRowLength(636) < imageExtent.width(640)` 也违反 VU（压缩图像要求 `imageExtent` 是块尺寸整数倍、`bufferRowLength` 是块宽整数倍且不小于 extent 宽）。
- 好消息：`api.h` 的 `FormatUtil_BitSizeOfBlock/WidthOfBlock/HeightOfBlock`（`:2460 / :2603 / :2687`）块信息齐全（未压缩默认 1×1，BC7=128bit，ASTC 6x6=128bit/6×6），`upload.cpp:126-138` 的 staging 大小已经按块算，不用重写。

## 3. 目标与非目标

**目标**

- `.ktx2`（以及内容其实是 KTX2 的 `.ktx`）全链路加载：未压缩、原生块格式、UASTC、ETC1S（含 BasisLZ / zstd 超压缩）。
- 按设备能力自动选转码目标，带确定性兜底（RGBA32）。
- cubemap（6 faces）、纹理数组、cube array、3D、1D（按 height=1 的 2D 上传）、完整 mip 链、非 2 的幂尺寸、sRGB 与 linear 语义正确。
- **最终上传格式**未压缩的贴图的运行时 mipmap 生成按 slice（层/面）逐条推进，覆盖 2D / cubemap / 2D array / cube array（判据是转码之后的 `isCompressed`，不是源文件的容器格式，见 §4.3 步 5）。
- 原生 `R8G8B8` 在缺 SAMPLE 位的设备上回退成同 transfer 的 RGBA8（CPU 展开，A=255）。
- 格式不支持时给出可执行的错误信息（差在哪个格式、用什么命令重打包）。

**非目标（明确边界，后续单独排期）**

- KTX1 容器：删分支，遇到就报错，让用户 `ktx2ktx2` 或 `ktx create` 重打包。
- 3D 的 mipmap：generate-mip pass 是 2D blit，`baseDepth > 1` 的文件一律按文件里的层上传，运行时生成与烘链都不走（工具侧 `--generate-mipmap` 本来也拒绝 3D，README §8.9）。
- **最终上传格式**是块压缩的贴图不做运行时 mipmap 生成（块格式不能当 generate-mip blit 的目标），要求烘进文件（README §5）。唯一的间接例外：可转码文件在该设备上落到 RGBA32 兜底时，最终格式是未压缩的，照常运行时生成（§4.3 步 5）。
- HDR（BC6H / ASTC float）：VkFormat 表会映射，但工具链没有 HDR 编码路径（README §3），只做映射不做验证承诺。
- `.basis` 容器、KTX2 video（`isVideo`）。
- 资产级"兄弟文件兜底"（§5.4），本期只记录方向。

## 4. 分层改动

### 4.1 构建层：`src/khr/xmake.lua`（已完成，实测通过）

```
add_includedirs(".")
add_cxflags("-fno-strict-aliasing", {tools = {"gcc", "gxx", "clang", "clangxx"}, force = true})
add_files("ktx/lib/basis_transcode.cpp")
add_files("ktx/lib/basisu/transcoder/basisu_transcoder.cpp")
```

体积/裁剪（可选，建议放在 loader 跑通之后再做）：默认全开时 debug `ktx.lib` 4.2 MB，其中常量表 `.inc` 约 1.7 MB。只保留引擎候选集（BC1/BC3/BC4/BC5/BC7、ETC2/EAC、ASTC 4x4、RGBA32、UASTC、ETC1S）时可以关掉 `BASISD_SUPPORT_ATC / BASISD_SUPPORT_FXT1 / BASISD_SUPPORT_PVRTC1 / BASISD_SUPPORT_PVRTC2`，去掉约 650 KB 表数据（atc 两个 inc 334 KB + pvrtc2 两个 inc 318 KB）与对应代码；关掉后 `basis_is_format_supported` 返回 false，libktx 侧表现为 `KTX_UNSUPPORTED_FEATURE`，正好被候选链跳过。

`src/khr` 未来会独立成包（AGENTS.md），本方案不新增对外形态：`ktx` 仍是只有 `pulse_graphics` 依赖的静态库。

### 4.2 RHI 层：cgpu（submodule，需单独提交）

1. `cgpu_vulkan.inl`：`VkUtil_FormatTranslateToVk` 补 BC1_RGB / BC1_RGBA / BC2 / BC3 / BC4 / BC5 / BC6H / BC7、ETC2 全族、EAC R11 / RG11、ASTC 全族（UNORM + SRGB）；`VkUtil_FormatTranslateToCGPU` 同步补反向分支。PVRTC 保持返回 UNDEFINED（没有对应扩展枚举保证）。
2. `cgpu_cmd_transfer_buffer_to_texture_vulkan`（`cgpu_vulkan_resources.c:300-343`）：块计数改向上取整，`imageExtent.width/height` 改成 `blocksX*blockW / blocksY*blockH`（压缩格式必须是块整数倍），未压缩路径保持等价。与 `upload.cpp` / `rendergraph.cpp` 的算法统一成一个约定：**向上取整**（`rendergraph.cpp:564-567` 和 `upload.cpp:44-46` 已经是对的，只有 cgpu 这边是 floor）。
3. 验证 BC6H/BC7 是否需要显式 enable `VK_EXT_texture_compression_bptc`。若验证层报 extension 未启用，把该扩展加进 `vulkan_utils.c:120-133` 的 promoted/wanted 名单；ASTC 是核心格式，不需要扩展。
4. `VkUtil_EnumFormatSupports` 不用改逻辑：表补好后 BC/ASTC 的 `CGPU_TEXTURE_FORMAT_SUPPORT_SAMPLE` 自然被填上，回退判定才可信。

### 4.3 loader 层：`load_texture.cpp` 的 KTX 分支重写

单次 step 内的线性流程（保持现有 PENDING → 等 upload_completed → DONE 两段式）：

1. `ktxTexture_CreateFromMemory(bytes, LOAD_IMAGE_DATA_BIT, &tex)`。
2. `tex->classId != ktxTexture2_c` → 失败，错误信息写"KTX1 不支持，用 `ktx create` 或 `ktx2ktx2` 重打包"。删掉 `glInternalformat` switch。
3. 结构闸门：只判 `numFaces == (isCubemap ? 6 : 1)`（面数与 cubemap 标志矛盾即失败）。**不再**判 `numDimensions == 2` / `baseDepth <= 1` / `numLayers > 1`——libktx 在 `texture2.c:667-691` 已把几何归一化完毕（1D 的 `pixelHeight==0` → `baseHeight=baseDepth=1`；`layerCount==0` → `numLayers=1`），loader 直接取 `baseWidth/baseHeight/baseDepth`，多维一律支持。`generateMipmaps` 不在这里拦——它是未压缩单级文件的合法运行时生成信号（`checkheader.c` 把 `levelCount==0` 归一为 `numLevels=1, generateMipmaps=1`），块压缩文件命中它则交给步 5 按**转码后的最终上传格式**决定（仍是块压缩才 warning，见步 5），不硬失败：libktx 加载路径不校验"压缩 + levelCount=0"，`error-3017` 只存在于离线 `ktx validate`。
4. 选目标格式（§5），必要时 `ktxTexture2_TranscodeBasis(tex, ktxTarget, flags)`；转码后 `tex->vkFormat` 会被替换成具体块格式、`isCompressed`/DFD 同步更新（`basis_transcode.cpp:352-374`），再走同一张映射表。注意转码会 `free` 旧 `pDfd` 换成 prototype 的，任何要读原始 DFD（模型名等）的值必须在调用 `TranscodeBasis` 之前捕获。
5. 建纹理：`array_size = numLayers * numFaces`（单 2D=1、单 cube=6、2D array=N、cube array=N*6、3D=1）、`depth = baseDepth`、`format` 来自上传格式判定（§5.3）。mip 策略是**三分支互斥**：`baseDepth > 1` 一律不生成（generate-mip 是 2D blit）；否则 `generateMipmaps && isCompressed` 只 warning 并按文件层上传；否则 `generateMipmaps` 时 `mip_levels = log2(max(w,h))+1`、加 `CGPU_RESOURCE_TYPE_RENDER_TARGET`，由 rendergraph 的 generate-mip pass **对每个 slice 各起一条链**（cube 按面、数组按层、cube array 按面×层），一次 `queue_staging_texture_full(..., source_mip_levels = numLevels, generate_mipmaps = true)`。这里的 `isCompressed` 是**最终上传格式**的性质而不是源文件的：判定发生在步 4 之后，`ktxTexture2_TranscodeBasis` 会用 prototype 的 formatSize 回写 `isCompressed`（`basis_transcode.cpp:360`），所以 UASTC/ETC1S 兜底到 RGBA32 的文件也算未压缩、会走运行时生成，转成 BC7/BC3/ASTC/ETC2 的仍按块压缩只 warning。`isCubemap` 额外置 `CGPU_RESOURCE_TYPE_TEXTURE_CUBE`。`PulseTextureLoadDesc::generate_mipmaps` 不参与 KTX 路径决策。
6. 上传：逐 `(level, layer, slice)` 用 `ktxTexture_GetImageOffset` + `ktxTexture_GetImageSize(ktx, level)` 整块 memcpy 到 staging 游标，`slice` 对 cubemap 是面、对 3D 是 depth slice（`ktx_slice_count` helper），源层数用 `numLevels`（运行时生成时只有 1 级）。必须逐 image 搬，不能整块 `dataSize` 一把梭：KTX2 每级按 `_requiredLevelAlignment`（`texture2.c:1663`，未压缩是 `lcm4(blockBytes)`）有 padding，而 staging 布局是紧密排列。**所有校验（image size 对得上 cgpu 块布局、逐层总量对得上 `texture_data_size`、GetImageOffset 成功）必须在 `queue_staging_texture_full` 之前完成**——入队后任何 FAILED 都会让 `pending_uploads` 残留指向已释放 loader state / texture 的条目（`clear_upload_pending` 只匹配 `UPLOAD_TEXTURE`，清不到 loader 用的 `UPLOAD_TEXTURE_DATA`）。入队后仅保留 `assert(written == totalSize)` 之类的不变量。删掉 `component`/`textureComponent` 双轨逻辑，映射表就是唯一事实来源。源布局与上传布局可能不同 bpp（§5.3 的 RGB8 回退），所以校验用**源格式**、建纹理与 staging 总量用**上传格式**，两个尺寸分别算，扩展路径逐 texel 把 3 字节写成 4 字节（A=0xFF），写序仍是 mip-major / slice-minor。
7. `ktxTexture_Destroy`（转码后仍安全，classId 分派）。
8. 扩展名 `ld2.extensions = "ktx,ktx2"`。注意 `asset_registry.cpp:52`：同一扩展名重复注册会直接失败，所以必须改现有 desc 的字符串，不能新增一个 loader 条目。
9. `pulse_create_texture` 路径格式无关、按块算大小，本身不用改；但它的 `pixel_data_size` 校验同样要在 `queue_staging_texture_full` **之前**做（见 §4.3-6 与 §11），否则失败会残留悬垂的上传条目。

### 4.4 映射表（vkFormat → ECGPUTextureFormat）

放在 loader（或 `graphics_internal.h` 旁的小工具）里，一个 switch，覆盖：

| 类别 | vkFormat | CGPU |
| --- | --- | --- |
| 未压缩 8bit | 23 `R8G8B8_UNORM` / 29 `R8G8B8_SRGB` | `R8G8B8_UNORM` / `R8G8B8_SRGB`（设备支持就按 24bpp 原生上传；缺 SAMPLE 位时按 §5.3 在 CPU 展开成同 transfer 的 RGBA8） |
| 未压缩 8bit | 37 / 43 `R8G8B8A8_UNORM/SRGB`、44 `B8G8R8A8_UNORM`、1 `UNDEFINED` | 同名（1 判失败） |
| HDR | 97 `R16G16B16A16_SFLOAT`、109 `R32G32B32A32_SFLOAT` | 同名 |
| BC | 131–146（BC1_RGB 131/132、BC1_RGBA 133/134、BC2 135/136、BC3 137/138、BC4 139/140、BC5 141/142、BC6H 143/144、BC7 145/146） | `BC1_RGB_*` … `BC7_*_BLOCK` |
| ETC2/EAC | 147–152 ETC2、153–156 EAC | `ETC2_R8G8B8*`、`EAC_R11*`、`EAC_R11G11*` |
| ASTC | 157–184（4x4…12x12，UNORM/SRGB） | `ASTC_*X*_UNORM_BLOCK` / `_SRGB_BLOCK` |

表必须包含全部 BC/ASTC 成员（原生块文件与转码结果都用它）。找不到映射 → 失败并报 vkFormat 数值。

## 5. 回退设计（核心）

设备能力来源：`cgpu_adapter_query_adapter_detail(device->adapter)`（`api.h:2280`）→ `format_supports[fmt] & CGPU_TEXTURE_FORMAT_SUPPORT_SAMPLE`（`api.h:1149`、`:766-778`）。`device` 已经通过 `ctx->user_data` 传进 loader（`load_texture.cpp:113`），`CGPUDevice.adapter` 是公开字段。

### 5.1 分支 A：可转码文件（`ktxTexture2_NeedsTranscoding`，即 DFD model == ETC1S 或 UASTC）

候选表按序尝试，第一个满足「设备支持 && 转码器支持 && 语义合法」的即胜出：

| 序 | `ktx_transcode_fmt_e` | 结果 vkFormat | 适用与理由 |
| --- | --- | --- | --- |
| 1 | `KTX_TTF_BC7_RGBA` | 145/146 BC7 | PC 首选，8bpp，ETC1S(mode5)/UASTC 都支持 |
| 2 | `KTX_TTF_BC1_OR_3` | 131/132 或 137/138 | libktx 自动按有无 alpha 选 BC1/BC3，sRGB 安全 |
| 3 | `KTX_TTF_BC5_RG` | 141 | 仅当 DFD transfer==LINEAR 且通道是 `UASTC_RRRG` / `ETC1S_GGG`（法线）；配 `KTX_TF_TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS`。**当前未进实现候选链**（与 BC7 同为 8bpp，收益低，见 §10） |
| 4 | `KTX_TTF_BC4_R` | 139 | 仅当 LINEAR 且单通道（`ETC1S_GGG` 之外的遮罩）。**当前未进实现候选链**（同上） |
| 5 | `KTX_TTF_ASTC_4x4_RGBA` | 157/158 | 移动/无 BC 设备；转码器只出 4x4（8bpp），省不了带宽 |
| 6 | `KTX_TTF_ETC2_RGBA` / `KTX_TTF_ETC` | 149/150 等 | Android |
| 7 | `KTX_TTF_RGBA32` | 42/43 | **无条件兜底**：`basis_is_format_supported` 对 ETC1S 直接返回 true（`basisu_transcoder.cpp:11064`），UASTC 是"除少数例外全支持"（`:11041-11055`）；`R8G8B8A8_SRGB/UNORM` 是 Vulkan 强制格式。代价 32bpp，必须打 warning |

要点：

- 序号 3/4 的存在理由：`ktx.h:1438-1441` 明确警告不要把 sRGB 数据转成 BC4/BC5/EAC（这些格式没有 sRGB 变体），所以它们只在 DFD transfer==LINEAR 时进候选。alpha/双通道信息从 DFD channel id 读，`basis_transcode.cpp:166-185` 已经把 `ETC1S_AAA/GGG`、`UASTC_RGBA/RRRG` 区分好了。
- 语义（sRGB/linear）不需要 loader 猜：转码结果 vkFormat 已带 `*_SRGB` 后缀（`basis_transcode.cpp:166,254,271`）。
- 过滤顺序：先 `format_supports` 的 SAMPLE 位过滤，再真正调 `ktxTexture2_TranscodeBasis`；返回 `KTX_UNSUPPORTED_FEATURE` / `KTX_INVALID_VALUE` 就试下一个候选，不 fatal。RGBA32 那行标 `always_available`，**跳过能力过滤直接试**——否则当 `cgpu_adapter_query_adapter_detail` 返回空（能力位全 0）时，兜底也会被 skip，违背"任何设备可加载"。
- 只在候选表全部失败（理论上不会，因为有 RGBA32）才 FAILED；每个 `return false` 分支都要写 `*out_error`，否则上层只剩通用的"asset loader step failed"。
- 记录一行日志：文件模型（ETC1S/UASTC）、原 vkFormat、选定目标、尺寸、层数、转码耗时。模型名/格式名必须在调用 `TranscodeBasis` **之前**从原始 DFD 捕获——转码会替换 `pDfd` 与 `vkFormat`。

### 5.2 分支 B：原生块格式文件（BC / ASTC / ETC2，vkFormat 已是具体块格式）

`format_supports` 有 SAMPLE → 直接上传（零转码成本，这是最快的路径）。没有 → **FAILED**，错误信息必须包含：

- 文件需要的格式名与尺寸；
- 设备缺这个能力；
- 正确的解决方案：**从该贴图的 UASTC / ETC1S 母文件重新出目标格式**，`ktx transcode --target <format>` 只能读可转码文件（README §8.8），对已是原生块格式的 `in.ktx2` 再转会报 `KTX file is not transcodable`。所以提示语不能写"把这个文件 transcode 一下"，要写"从母格式重新 transcode"。

理由：运行时没有 BC/ASTC 解码器，libktx 也没有提供（`etcdec.cxx` 未参与编译，`astc_encode.cpp` 是编码器），这不是"还没做"而是"做不了低成本回退"。要跨平台就喂可转码格式。

### 5.3 分支 C：未压缩 vkFormat

直接映射 + `format_supports` 检查，支持即按原生格式上传。`R8G8B8_UNORM/SRGB` 不是 Vulkan 强制格式，个别驱动（实测 Intel 集显）缺 SAMPLE 位——此时**不再报错**，退到同 transfer 的 `R8G8B8A8_UNORM/SRGB`：`R8G8B8` 是 1×1 块 24bpp、`R8G8B8A8` 是 32bpp，texel 数一致，KTX2 未压缩 image 又没有行填充（行 4 对齐是 KTX1 的事），所以上传前在 CPU 上逐 texel 把 3 字节抄成 4 字节、A 填 255 是无损的，语义（sRGB/linear）由 transfer 保证不变。代价是一次 CPU 展开与 1/3 带宽/显存，所以资产作者仍建议直接出 RGBA8。判定只发生在 `resolve_upload_format` 一处，并把「源格式 / 上传格式 / 是否需要展开」一起交回 `step_texture_ktx`，避免在拷贝循环里再猜一次格式。

其它缺 SAMPLE 的未压缩格式（如 `R8G8`、`R16` 之类）没有同语义的必支持替身，**不做**顺手兜底，维持报错——只有语义严格等价且目标是 Vulkan 强制格式的回退才有根因支撑。

### 5.4 可选（二期，不进本期）：资产级兄弟兜底

`pulse_asset` 已有 loader 依赖机制（`pulse_asset_load_task_add_dependency` + `PULSE_ASSET_LOADER_STATUS_WAIT_DEPENDENCIES` + `PULSE_LOAD_DEPENDENCY_REQUIREMENT_OPTIONAL`，`pulse_asset/include/pulse_asset.h:489`、`:60-61`）。可以让 `.ktx2` 通过 kvData（如 `pulse.fallback = "textures/albedo.png"`）声明可选兜底资产，格式不支持时借用兜底纹理。代价是同一逻辑贴图可能占两份 GPU 资源、生命周期变复杂；先把 §5.1/§5.2 的确定性策略做扎实，这条留到有真实需求再说。

## 6. 性能与线程约束

- `basisu_transcoder_init()` 在首次转码时跑，libktx 注释标称约 9 ms（`basis_transcode.cpp:333-340`，内部 static 保证只跑一次）。
- ETC1S→BC1/BC3 快；UASTC→BC7/ASTC 明显慢（1K 量级几 ms～几十 ms，全 mip 链）。
- loader step 跑在主更新循环（`src/pulse_asset/src/asset_loading.cpp:330`），每 update 最多 `max_requests_per_update = 8` 个 job（`asset_plugin.cpp:24`）→ 单张贴图卡一帧可接受，首帧批量加载 UASTC 会明显掉帧。缓解：把 step 拆成"解析+转码 → 上传"两次 PENDING（转码放第一次，仍是一帧）；要真正并行得先给 `pulse_asset` 加 worker 池，不在本期。
- 内存峰值：转码会先建 `prototype`（目标格式全 mip 空间），成功后把 data 换进 `This` 并释放原 data（`basis_transcode.cpp:290-382`）→ 峰值 ≈ 源 + 目标，可接受。
- 正解仍是：**能离线定的格式就别在运行时转**。发布管线建议 `UASTC(母) → ktx transcode → BC7/BC3` 出 PC 包，ASTC 6x6/8x8 出 Android 包（README §4.2、§4.4），运行时转码只当兼容与开发期能力。

## 7. 验证方案

### 7.1 测试资产（用 `tools/ktx` 生成，放 `tests/graphics/data/`，命令写进测试目录）

| 文件 | 生成命令要点 | 覆盖 |
| --- | --- | --- |
| `tiles_rgba_srgb.ktx2` | `ktx create --format R8G8B8A8_SRGB --generate-mipmap` | 未压缩 43（当时必然失败的那条，现在入库为 `Tiles130_rgba_srgb.ktx2`，属必载组） |
| `tiles_rgb_srgb.ktx2` | `--format R8G8B8_SRGB` | 29，设备支持时按 24bpp 原生上传，缺 SAMPLE 位时按 §5.3 展开成 RGBA8 |
| `tiles_bc7.ktx2` | `--encode uastc` 后 `ktx transcode --target bc7` | 分支 B 原生 BC7 |
| `tiles_uastc.ktx2` | `--encode uastc --uastc-quality 2 --uastc-rdo --generate-mipmap` | UASTC → BC7 转码 |
| `tiles_etc1s.ktx2` | `--encode basis-lz`（可加 `--qlevel 128`） | ETC1S/BasisLZ → BC1/BC3 |
| `tiles_uastc_zstd.ktx2` | UASTC + `ktxsc --zcmp 19` | zstd 超压缩 |
| `tiles_astc6x6.ktx2` | `--format ASTC_6x6_SRGB_BLOCK` | 原生 ASTC（桌面负例、Android 正例） |
| `tiles_nonpot.ktx2` | 640×480 的 UASTC 与 ASTC 各一份 | 块向上取整 / cgpu extent 修复 |
| `tiles_normal.ktx2` | `--format R8G8B8A8_UNORM --assign-oetf linear --normal-mode` | LINEAR + 双通道 → BC5 候选 |
| 现成样本 | `src/khr/ktx/lib/basisu/webgl/ktx2_encode_test/assets/kodim23.ktx2`（实测 UASTC + `KTX_SS_ZSTD`，768×512，10 级） | 免生成即可验 UASTC+zstd |

`tests/graphics/data/TilesGray512.ktx` 已经是 KTX2 + vkFormat 23（`R8G8B8_UNORM`），不需要重打：`vk_format_to_cgpu` 现在把 23 正确映射成 `CGPU_TEXTURE_FORMAT_R8G8B8_UNORM`（linear），不再把它当 sRGB 上传；本机这类缺 `R8G8B8` SAMPLE 位的设备再按 §5.3 展开成 `R8G8B8A8_UNORM` 加载（实跑证据见 §12）。它因此成了 linear RGB8 的回归样本，与 `Tiles130_rgb_srgb.ktx2`（sRGB 那条）互补。

### 7.2 断言与用例

- 每个资产：`pulse_texture_is_ready` 为真；`texture->handle->info->format` 等于期望格式；渲染 0 级并与 `baseline.png` 比对（现有 `xmake/rules/window_screenshot` + skill `test-tool-for-program-with-window`；带窗口 target 先 build 再按 skill 跑）。
- 一致性：同一张源图，PNG(stb) / KTX2 未压缩 sRGB / UASTC→BC7 三条路径像素在容差内一致（现在 README §9 就吐槽过颜色对不上）。
- 回退用例：临时把候选表截断到 RGBA32（配置或编译期开关，测试后不改默认），断言贴图仍成功、格式是 `R8G8B8A8_SRGB/UNORM`，有 warning。
- 负例用例：原生 ASTC 文件在不支持 ASTC 的设备上失败，错误串包含格式名与 `ktx transcode` 提示。
- 越界回归：非块整数倍尺寸仍给错误信息而不是崩；数组 / 3D / cube array 已从"拒绝路径"变成正例，靠入队前的逐 image size + 逐总量两道校验保证多维 staging 布局不越界（历史 bug 是 `arraySize` 只认 1/6 却按 `numLayers * numFaces` 遍历写入）。
- RGB8 回退：设备缺 SAMPLE 位时断言仍 loaded 且 `info->format` 是对应的 `R8G8B8A8_*`；同一张源图的 `*_rgb_srgb.ktx2` 与 `*_rgba_srgb.ktx2` 渲染结果应逐像素一致（两文件的 level0 数据实测 SHA256 相同，可直接当基准）。
- 全量：`xmake test`（不要用 `xmake build` 构建测试目标）。
- 打开 `enable_debug_layer`（仓库带 `vkvalidate/lib`）跑一遍，确认没有 `vkCreateImage` 格式相关、`VUID-VkBufferImageCopy-imageExtent-*`、`bufferRowLength` 的报错。

### 7.3 平台矩阵

Windows/NVIDIA（BC7）、Windows/AMD、Android（ASTC + ETC2）、无 ASTC 的设备（走 §5.2 报错路径）、软件实现（若可用，验 RGBA32 兜底）。

## 8. 落地顺序（每步都能编译、能跑）

1. `src/khr/xmake.lua` 加转码器源码 + include 路径 + `-fno-strict-aliasing`。**已完成并实测**（MSVC 2.25 s 编译、`ktx.lib` 里 `ktxTexture2_TranscodeBasis` / `basis_is_format_supported` / `basisu_transcoder_init` / `ktxTexture2_NeedsTranscoding` 齐全、`pulse_graphics.dll` 链接通过）。
2. cgpu：补格式翻译表（双向）+ 修 `transfer_buffer_to_texture` 块取整；submodule 提交 → 父仓更新 gitlink。
3. loader：映射表 + KTX2 主干 + 结构闸门 + 按块拷贝 + `ktx,ktx2` 扩展名 + 合并重复的 `TextureLoaderState`。此时未压缩 43/37/23/29 与原生 BC7/ASTC 应该已经可用。
4. loader：转码候选链 + 回退 + 日志。此时 UASTC / ETC1S 可用。
5. 测试资产 + 用例 + VVL 验证（§7）。
6. 文档收尾：把 `tools/ktx/README.md` §9「引擎侧现状」改写成「支持矩阵 + 推荐打包命令」，勾掉 `docs/v0.3/版本任务.md` 第 3 项。

## 9. 风险与注意事项

- cgpu 是 submodule，表改动必须进 fork，父仓与子仓成对提交；`git status` 现在就有 `M cgpu`（gitlink 已偏移），提交前先看一眼。
- BC6H/BC7 与 ASTC HDR 依赖驱动/扩展，必须靠 `format_supports` 实测，不能按"桌面一定有"写死。
- 转码目标 ASTC 固定 4x4：给移动包想要 6x6/8x8 的体积优势只能离线出（README §3、§4.4）。
- `--runtime-mipmap`/`--automipmap`（`levelCount=0`）用在压缩贴图上会产出非法文件（README §5），但**离线 `ktx validate` 才报 `error-3017`；libktx 的加载路径不拦**——`checkheader.c:267-271` 把 `levelCount==0` 归一为 `numLevels=1` 并置 `generateMipmaps=1`。所以 loader 里 `numLevels < 1` 恒不成立（死条件），拦截/降级必须判 `ktx->generateMipmaps`，不能判 `numLevels`。
- 转码在主线程，首帧批量加载会掉帧（§6）；不要把"运行时转码"当发布策略。
- 块尺寸向上取整是三处（cgpu / upload.cpp / rendergraph.cpp）的共享约定，改一处就要对齐另两处。

## 10. 实施结果（2026-07 验证记录）

§8 六步全部落地，`xmake test` 58/58 通过，`test-graphics` 开 VVL 跑通。测试资产实际落在 `tests/graphics/data/`（130×130 源、8 级 mip）：`Tiles130_rgba_srgb / rgb_srgb / uastc / uastc_zstd / etc1s / bc7 / astc6x6`，全部 `ktx validate` 通过。探测断言在 `tests/graphics/main.cpp`：必载组（未压缩 / UASTC / UASTC+zstd / ETC1S）任何设备必须 loaded；`TracksFormatSupport` 组（BC7 / ASTC6x6 / RGB8 / 现成 `TilesGray512.ktx`）断言「加载结果 == 设备 SAMPLE 位」，设备能力在 renderer 存活期（请求时）快照——`report` 跑在 `pulse_app_run` 返回后，那时 `renderer.adapter` 已被置空，退出后再查表会全 0。

与 §7.1 计划的差异：

1. zstd 资产改用 `ktx create --encode uastc --zstd 19` 生成；`ktxsc --zcmp` 对已有 UASTC 会产出重复 `KTXwriterScParams`（`error-7013`，`ktx validate` 判非法），该路径不可用。
2. `--encode basis-lz --generate-mipmap` 的完整 mip 链会产出末两级 byteLength 逆序的非法文件（`error-4001`），实测须 `--levels 8` 截断。
3. BC5/RG 候选（`tiles_normal.ktx2` 那条）本期未进候选链：BC5 与 BC7 同为 8bpp，收益低；运行时 mip 的处理见 §11（不再"报错拦截"，改成按格式判定后决定生成或 warning）。
4. VVL 实测确认了拷贝规则的方向：`imageExtent` 必须用真实 mip 尺寸（130），**不能**按块向上取整；块取整只属于 `bufferRowLength/bufferImageHeight`。首版做反（extent=132 > image=130 触发 `VUID-vkCmdCopyBufferToImage-imageSubresource-07971`），已按此修正。
5. 本机（Intel 集显，无 ASTC/RGB8 SAMPLE）实测：UASTC→BC7 ≈ 40 ms、UASTC+zstd ≈ 1 ms、ETC1S ≈ 0 ms（开 VVL）。

## 11. 复核修正（2026-08）

对照 libktx / cgpu 源码复核 §8 产物，修掉以下问题，`xmake test` 58/58 通过、`test-graphics` 开 VVL + GPU-based validation 跑通。

- **运行时 mipmap（推翻 §4.3 早先"压缩文件报错"的写法；本条的 `!isCubemap` 限制又被 §12 放开）**：loader 之前判的死条件 `numLevels < 1` 永不成立（见 §9），改成 `generateMipmaps && !isCompressed && !isCubemap` 时走引擎自己的 generate-mip pass 生成链，压缩 / cubemap 只 warning 并按文件里的层上传。`load_desc->generate_mipmaps` 不再参与 KTX 路径决策（是否生成由文件的 `levelCount==0` 决定）。新增必载资产 `Tiles130_runtime_mip.ktx2`（`ktx create --format R8G8B8A8_SRGB --runtime-mipmap`，`levelCount=0`）覆盖这条路；实测日志 `generates 8 mips from the stored level at upload`。
- **入队前完成校验**：原 loader 在 `queue_staging_texture_full` 之后还有 image layout 的 FAILED 分支，会残留 `pending_uploads` 指向已释放 loader state / texture（`clear_upload_pending` 清不到 `UPLOAD_TEXTURE_DATA`）。改为 size/offset 校验全部前移到入队之前，入队后只剩 `assert` 不变量。`create_texture.cpp` 的 `pixel_data_size` 校验同样前移。
- **RGBA32 兜底真正无条件**：候选表加 `always_available`，末位跳过能力过滤，命中时打 32bpp warning。
- **报错文案**：`vkformat_str.c` 编进 `ktx`，格式用 `vkFormatString` 打名字并带尺寸；原生块格式不支持时提示"从 UASTC/ETC1S 母文件 transcode"，不再误以为能直接转这个文件。每个失败分支都写 `*out_error`。
- **日志**：转码那行补齐模型（uastc/etc1s）+ 源 vkFormat → 目标 vkFormat + 尺寸 + 层数 + 耗时；模型/源格式在 `TranscodeBasis` 前捕获（转码会替换 `pDfd`）。
- **候选链语义修正**：删掉 `KTX_UNSUPPORTED_TEXTURE_TYPE`（不是 `TranscodeBasis` 的返回值），保留 `KTX_UNSUPPORTED_FEATURE`/`KTX_INVALID_VALUE` 作换候选信号。

未做（留后续）：MustLoad 探针断言实际转码目标格式、候选表截断到 RGBA32 的兜底用例。（当时列为待做的 cubemap / 纹理数组 / 3D 资产与加载、RGB8 缺 SAMPLE 位时展开成 RGBA8，已在 §12 落地。）

## 12. 多维与 RGB8 回退（2026-08 第二轮）

推翻本文早先"只支持 2D / 单 cube"的边界，本轮落地 `array / 3D / cube array / 1D` 加载与逐 slice 运行时 mip，以及 RGB8 设备回退。`tools/ktx/README.md` §9 的支持矩阵已同步。

- **几何**：`width/height/depth` 直接取 libktx 归一化后的 `baseWidth/baseHeight/baseDepth`，`array_size = numLayers * numFaces`。1D（`pixelHeight==0`）由 libktx 归一为 `baseHeight=baseDepth=1`，再由 `init_texture` 已有的 `FORCE2D` 变成 height=1 的 2D 纹理——全仓只有这一条 1D→2D 通路，loader 里不加第二套映射。原来的 `numDimensions == 2` / `baseDepth <= 1` / `numLayers > 1` 三道拒绝式闸门删除，只保留 `numFaces == (isCubemap ? 6 : 1)` 这条一致性校验。
- **cube 的层数口径**：cube array 的 `array_size = numLayers * 6` 只要求是 **6 的正整数倍**，18 层（3 层 cube array）、30 层都合法——不是 12 的倍数。定调依据是本机 Vulkan SDK 的 `validusage.json`：`viewType-02960` 要求 `CUBE` 视图 `layerCount` **恰好 6**，`viewType-02961` 只要求 `CUBE_ARRAY` 视图是 6 的倍数；两条都写成 `cgpu_create_texture_view_vulkan` 里 dims switch 的 `cgpu_assert`，`is_cube` 前置校验对应 `image-01003`；`01004`（`imageCubeArray` feature）不用操心——cgpu 建 device 时把查到的 `VkPhysicalDeviceFeatures` 整体当 `pEnabledFeatures`（`cgpu_vulkan_instance.cpp`），设备支持就是已开启。整图视图维度由唯一一份派生决定：`HGEGraphics::texture_view_dims`（`init_texture` 与 `pulse_render_pass_encoder_resolve_texture_view` 共用；subresource 视图不进这个函数，cube 的面固定是 `2D`，这正是逐面 blit 的合法性来源）。层数与 mip 数的边界来自 RHI 的 `uint8_t`：`CGPUTextureViewDescriptor.base_array_layer` / `array_layer_count` 在 cgpu 里就定义为 `uint8_t`（既有约束，不是本轮新加的截断），rendergraph 的 `ResourceNode.mipCount` / `arraySize` 同宽。**第一处窄化其实在 `init_texture` 里**：它把 `uint32_t arrayCount` 赋给整图视图的 `array_layer_count`（`renderer.cpp`），是隐式转换，**任何构建模式都静默截断**；顺序上 `texture_view_dims` 的 `% 6` 断言跑在这之前（用的是未截断值），`pulse_render_graph_import_texture_impl` 的 `mip_levels <= 0xFF && array_size_minus_one + 1 <= 0xFF` 断言跑在这之后。所以可用上限是 **255 层 / 255 级**，且"越界会被抓住"只能是 **debug 断言保护**：`assert` 与 cgpu 的 `cgpu_assert` 在 `NDEBUG` 下都会空掉（`common_utils.h` 里退化为 `(void)(expr)`），**release 构建不做任何保证**，越界会拿着回绕后的层数建出错误视图（例：258 层 cube array 过 `% 6` 但被截成 2）。同一族截断点还有 loader 入队上传时的 `uint8_t source_mip_levels`（`queue_staging_texture_full` 的入参，调用处是显式 `static_cast<uint8_t>`）。这里不加非 assert 的兜底分支，越界属于资产/调用方错误，由 debug 期暴露（这条边界未构造过越界资产去实测，属代码语义。存活证据的口径也要注意：import 断言与 `texture_view_dims` 的 `%6` 都**无 message**，而"在 exe 里扫到断言字符串"这类取证只对**带 message** 的 assert 有效（MSVC 的 `assert` 不发射 `#exp`/`__FILE__` 字面量，无 message 者在二进制里没有可判别对象），所以依据是"debug 构建不定义 `NDEBUG`"这一构建事实，不是字符串取证）。
- **mip（推翻 §4.3 步 5 早先的 `!isCubemap` 限制）**：三分支互斥——`baseDepth > 1` 永不生成（generate-mip pass 是 2D blit，且有 `assert(textureNode.depth == 1)`；loader 侧命中这条时连 `CGPU_RESOURCE_TYPE_RENDER_TARGET` 都不挂，因为 `descriptors` 只跟 `generate_mipmaps` 走）；否则**最终上传格式**未压缩的 2D / cubemap / 2D array / cube array 全部运行时生成（判据是转码之后的 `ktx->isCompressed`，见 §4.3 步 5：UASTC/ETC1S 落到 RGBA32 兜底时按未压缩处理、会生成链，落到 BC7/ASTC/ETC2 时仍按压缩只 warning），`pulse_render_graph_add_generate_mipmap` 去掉 `assert(arraySize == 1)`，对每个 slice 单独 declare `(from_mipmap-1, slice)` 作链头并独立推进；否则（压缩）保持 bake-chain warning。slice 语义：cube 是面、数组是层、cube array 是面×层、3D 是 depth slice。**3D 那道守卫已被实测坐实**：`tex_3d_32x16x8_rt.ktx2`（`R8G8B8A8_SRGB` 32×16×8、`levelCount=0`）走进去只打一次 `mipmaps are never generated for 3D`、没有任何 generates-mips 行，见下面「读回断言与证据边界」。
- **RGB8 回退**：见 §5.3。实现上把"源布局"与"上传布局"拆成两个格式（`UploadLayout`），逐 image / 逐总量两条 release 校验继续按**源格式**判（文件布局自洽性不因回退而放松），建纹理、staging 总量与写入步进按**上传格式**，扩展只在 `resolve_upload_format` 判定出的那一条路径上发生，A 填 0xFF。
- **测试资产**：`tests/graphics/data/` 新增 `tex_1d_256 / tex_3d_32x16x8 / tex_array4_64 / tex_cube_64 / tex_cube_array2_64.ktx2`，全部用 `tools\ktx\ktx.exe create` / `toktx.exe` 生成并 `ktx validate` 通过；后三者 `levelCount=0`，正是运行时生成路径。收尾再补两张：`tex_rgb8_64_rt.ktx2`（`R8G8B8_SRGB` + `--runtime-mipmap`）把"CPU 展开成 RGBA8"与"GPU 逐 slice 生 mip"两条语义压在同一条纹理上；`tex_3d_32x16x8_rt.ktx2`（`R8G8B8A8_SRGB` 32×16×8 + `--runtime-mipmap`，`levelCount=0`）专门喂 `depth > 1` 那道守卫——早先的 `tex_3d_32x16x8.ktx2` 是 `levelCount=1`，`generateMipmaps` 恒为 0，分支根本进不去，守卫被删也不会让任何探针变红。
- **实测（Intel 集显，缺 ASTC 与 R8G8B8 的 SAMPLE 位）**：`Tiles130_rgb_srgb.ktx2`（`R8G8B8_SRGB` 130×130×8 级）与 `TilesGray512.ktx`（`R8G8B8_UNORM` 512×512×10 级）都命中回退并加载成功，两条 transfer（sRGB / linear）各覆盖一个。其中 `Tiles130_rgb_srgb.ktx2` 与 `Tiles130_rgba_srgb.ktx2` 的 `ktx extract --level 0` 产物**SHA256 相同**（`9DF5C004…2DB4`，26946 B），即两条路径的 level0 像素逐字节一致，可作为回退正确性的比对基准。
- **读回断言与证据边界**：多维与 RGB8 的探针已接进 `tests/graphics/main.cpp` 的 `kKtxProbes`（`tex_1d_256 / tex_3d_32x16x8 / tex_3d_32x16x8_rt / tex_array4_64 / tex_cube_64 / tex_cube_array2_64 / tex_rgb8_64_rt`），早先担心的 `TracksFormatSupport` 语义冲突由新增的第三档 `LoadsEitherFormat`（恒 loaded，覆盖有等价回退的 `r8g8b8` 通道）消掉。取证强度已从"日志比对"升级为**测试内硬断言**：`check_ktx_probe_readbacks` 在"全部探针 resolved"的那一帧统一 `pulse_render_graph_import_texture` 一次（一次性 latch，不进稳态每帧路径），再用 `pulse_render_graph_texture_get_width/height/depth/format` 读回真实 `info`——dims 逐条硬断（1D 读回 `256x1x1`、两张 3D 读回 `32x16x8`、array / cube / cube array 读回 `64x64x1`），格式按 mode 断言集合，`LoadsEitherFormat` 必须命中同 transfer 的 RGBA8 侧（`Tiles130_rgb_srgb→r8g8b8a8_srgb`、`TilesGray512.ktx→r8g8b8a8_unorm`、`tex_rgb8_64_rt→r8g8b8a8_srgb`，即回退真的作用到了上传格式上）。`report_ktx_probes` 里的 `assert(!loaded || readback_checked)` 把"断言静默不执行"的空档也堵了。开校验层实跑：`16 checked, 0 failed`（读回 15 条 + latch 1 条；未加载的 `astc6x6` 按 `TracksFormatSupport` 不参与读回，它的 `FAILED: format not supported by device` 是预期日志、不计入 failed）；`tex_3d_32x16x8_rt` 那条守卫 printf 出现且仅出现一次、该资产无任何 generates-mips 行——"3D 不生成 mip"从代码推断变成有执行证据。两条证据口径分开记：截图与 `tests/graphics/baseline.png` **逐字节相同（similarity 1.0）**是补读回断言那一轮的取证，补 3D `_rt` 探针这轮只复核了探针与守卫日志、没有重跑像素比对，别混成一次实跑。**仍然成立的边界只有两条**：`mip_levels` / `array_size` 没有 capi getter，只能继续靠 loader 的 `generates N mips for M slices` 日志取证；逐资产的**渲染像素**没进基线（渲染基线只有当前场景那一张），内容等价靠 `ktx extract --level 0` 的 SHA256 oracle 证明。

## 13. loader 设置段的两条不变式（设计护栏）

`step_texture_ktx` 里"`upload_requested` 为假"的那一段（`ktxTexture_CreateFromMemory` → 格式判定 → 校验 → `init_texture` → staging 入队）有两条形同契约的约束。**当前代码都满足它们，这里是把它们钉住，不是记录缺陷。**

- **A. 设置段中途不得返回 `PULSE_ASSET_LOADER_STATUS_WAIT_DEPENDENCIES`。** `asset_loading.cpp` 的 `process_processing` 收到该状态会把 slot 置 `WAITING_DEPENDENCIES`、job.phase 切到 `WaitingDependencies`（队列把该 job 挪到队尾，`drain_pass` 靠 phase 变化判断有进展），依赖就绪后**从 `step` 顶部重新进入**（`process_waiting_dependencies` → `process_processing`）。设置段没有中途回滚点：闸门只有 `upload_requested`，而它要到段末才置位，所以从 `ktxTexture_CreateFromMemory` 之后的任何让出都会整段重跑——上一次的 `ktx` 对象泄漏（各 FAILED 分支都显式 `ktxTexture_Destroy`，让出分支不会）、几十毫秒的 Basis 转码白做，若已越过 `init_texture` / 入队则变成第二张 cgpu 纹理 + 第二条上传条目。结论：设置段必须一次跑完，要等依赖只能在它开始之前等完。§5.4 的资产级兜底二期若要做，必须先把设置段拆成分步幂等的状态机（每步一个已完成的标志位），不能直接塞一个 `WAIT_DEPENDENCIES`。
- **B. `queue_staging_texture_full` 是设置段最后一个副作用，其后只允许填 staging、置 `upload_requested = true`、`return PENDING`。** step 一旦返回 FAILED，`process_processing` 立刻 `job.finish(..., LoadJobOutcome::Failed, error)` 终结 job，主循环随即 `retire_and_erase` → `retire_load_job`：`loader_state.reset()` 释放 loader state（ktx loader 的 `dtor` 是 `nullptr`，但 state 内存照样回收），随后 `try_unload_slot` 回收 slot；而 `pending_uploads` 里那条 `UPLOAD_TEXTURE_DATA` 存着 `bool* completed`（指向已释放的 loader state）与 `texture` 指针，`clear_upload_pending` 只匹配 `UPLOAD_TEXTURE` / `UPLOAD_BUFFER`，清不到它 → 悬垂。§11 的"入队前完成校验"就是这条的历史 bug（当时入队后还留着 image layout 的 FAILED 分支），所以逐 image size / 逐总量 / offset 三道校验必须全部前移到入队之前，入队之后只保留 `assert` 级别的不变量。`pulse_create_texture` 路径的 `pixel_data_size` 校验同理（§4.3 步 9）。

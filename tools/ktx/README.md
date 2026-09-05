# tools/ktx 贴图压缩工具使用说明

本目录是 [KTX-Software](https://github.com/KhronosGroup/KTX-Software) 的预编译命令行工具，用于把 PNG/JPEG 等图片打包成 GPU 可直接采样的 `.ktx2`（BC / ASTC / Basis Universal 压缩贴图）。对应库源码在 `src/khr/ktx`（libktx + astcenc + Basis Universal）。

版本：`ktx.exe` v4.3.2~6，`toktx/ktx2ktx2/ktxsc/ktxinfo/ktx2check` v4.3.0~28。

| 程序 | 用途 |
| --- | --- |
| `ktx.exe` | 统一入口（首选）。子命令：`create` `encode` `transcode` `extract` `info` `validate` |
| `toktx.exe` | 老牌打包工具，图片 → KTX1/KTX2。cubemap / 数组 / 3D / 逐层输入更直观，OETF 自动处理 |
| `ktx2ktx2.exe` | KTX1 → KTX2 |
| `ktxsc.exe` | 对已有 KTX2 追加 zstd 超压缩（`--zcmp`），或把未压缩 KTX2 直接编成 ASTC / ETC1S / UASTC |
| `ktxinfo.exe` | 打印 KTX/KTX2 头信息（等价 `ktx info`） |
| `ktx2check.exe` | KTX2 结构校验（只做结构检查，不做语义检查，见 §6） |

查全部参数用 `ktx <子命令> --help` 或 `toktx --help`。注意 `ktx help create` 依赖未随仓库分发的 html 文档，会报 `Failed to open the html documentation`，用 `--help` 即可。

### 6 个 exe 都依赖同目录的 `ktx.dll`

它们都是薄壳，功能在 `ktx.dll`（libktx）里。实测：把任一 exe 单独拷到没有 `ktx.dll` 的目录运行，全部以 `0xC0000135`（STATUS_DLL_NOT_FOUND）失败。**分发/CI 时 `ktx.dll` 必须和 exe 放在一起**，不要只拷 `ktx.exe`。

### 能不能只用 `ktx.exe`？—— 大部分能，但有 5 件事它做不到

`ktx.exe` 的 `create/encode/transcode/extract/info/validate` 覆盖了日常打包，实测以下仍是独占能力：

| 只有独立 exe 能做 | 命令 | `ktx.exe` 的报错 |
| --- | --- | --- |
| 产出 **KTX1**（`.ktx` 容器） | `toktx out.ktx in.png` | `ktx create` 永远写 KTX2；即使输出名写成 `out.ktx`，文件头仍是 `«KTX 20»` |
| 给已有 KTX2 只加 **zstd 超压缩** | `ktxsc --zcmp 19 -o out.ktx2 -f in.ktx2` | `ktx encode --zstd 19 …` → `Missing codec argument` |
| 已有 KTX2 **→ ASTC**（不回到图片重编） | `ktxsc --encode astc --astc_blk_d 6x6 -o out.ktx2 -f in.ktx2` | `ktx encode --codec astc` → `Invalid encode codec: "astc"` |
| **读 KTX1** | `ktx2ktx2 in.ktx`、`ktxinfo in.ktx`、`ktx2check in.ktx` | `ktx extract/transcode in.ktx` → `fatal-2001: Not a KTX2 file`；`ktx info/validate` 对 KTX1 只报 Validation failed |
| **缩放**（`--resize WxH` / `--scale v`）、netpbm 输入、`--2d`、`TOKTX_OPTIONS` | `toktx --t2 --resize 64x64 out.ktx2 in.png` | `ktx create --resize` → `Option 'resize' does not exist` |

另两点行为差异：`ktx validate` 只接受**一个**文件，`ktx2check` 可以一次传多个（但它只做结构检查，见 §6）；`toktx` 的选项名用下划线（`--genmipmap`、`--assign_oetf`），`ktx.exe` 用连字符（`--generate-mipmap`、`--assign-oetf`），别混用。

日常建议：**打包统一用 `ktx.exe`**（ASTC/Basis + `transcode` 出 BC），只有在需要 KTX1、批量结构检查、或对已有 ktx2 做 zstd/ASTC 二次处理时才用独立 exe。

## 1. 支持的输入图片格式

实测结论：

| 输入 | `ktx create` | `toktx` |
| --- | --- | --- |
| PNG、JPEG | ✅ | ✅ |
| netpbm（`ppm` `pgm` `pam`） | ❌ `Image buffer too small` | ✅ |
| **BMP、TGA** | ❌ `No image plugin recognized the format` | ❌ 同左 |
| KTX / KTX2 | ❌ 不能当图片输入（KTX1 用 `ktx2ktx2`，KTX2 用 `ktxsc` / `ktx encode`） | ❌ |
| 裸数据（含 BC 块数据） | ✅ `--raw --width --height` | ❌ |

需要 netpbm 或逐层输入时用 `toktx`，其余场景两者都能出图。

所以 BMP 必须先转 PNG/JPEG，任选一种：

```powershell
magick in.bmp out.png
python -c "from PIL import Image; Image.open('in.bmp').save('out.png')"
pwsh -c "Add-Type -AssemblyName System.Drawing; [System.Drawing.Image]::FromFile('in.bmp').Save('out.png',[System.Drawing.Imaging.ImageFormat]::Png)"
```

## 2. 目标格式怎么写：VkFormat 名

`ktx create --format <enum>` 用 Vulkan 格式名，`VK_FORMAT_` 前缀可省，大小写不敏感。**块压缩格式必须带 `_BLOCK` 后缀**，写成 `BC7_UNORM` 会报 `The requested format is invalid or unsupported`。

| 目标 | `--format` 写法 | bpp |
| --- | --- | --- |
| RGBA8 未压缩 | `R8G8B8A8_SRGB` / `R8G8B8A8_UNORM` | 32 |
| RGB8 未压缩 | `R8G8B8_SRGB` | 24 |
| BC1（无 alpha） | `BC1_RGB_UNORM_BLOCK` / `BC1_RGB_SRGB_BLOCK` | 4 |
| BC1（1bit alpha） | `BC1_RGBA_UNORM_BLOCK` / `BC1_RGBA_SRGB_BLOCK` | 4 |
| BC3 / BC4 / BC5 | `BC3_SRGB_BLOCK` / `BC4_UNORM_BLOCK` / `BC5_UNORM_BLOCK` | 8 / 4 / 8 |
| BC7 | `BC7_UNORM_BLOCK` / `BC7_SRGB_BLOCK` | 8 |
| ASTC 4x4 ~ 12x12 | `ASTC_6x6_UNORM_BLOCK` / `ASTC_6x6_SRGB_BLOCK` 等 | 8.00 ~ 0.89 |

sRGB 与 linear 由你选的格式名决定，工具不会替你推断（对 ASTC/BC 尤其明显：`ASTC_8x8_UNORM_BLOCK` 产出的 DFD transfer 是 LINEAR）。颜色贴图选 `*_SRGB_BLOCK`，法线/遮罩等线性数据选 `*_UNORM_BLOCK`。

## 3. 能力矩阵（关键）

| 目标格式 | 能否直接从图片编码 | 路径 |
| --- | --- | --- |
| ASTC（4x4…12x12，含 3D 块） | ✅ | `ktx create --format ASTC_*_BLOCK` 或 `toktx --encode astc --astc_blk_d` |
| UASTC | ✅ | `ktx create --encode uastc` / `toktx --encode uastc` |
| ETC1S / BasisLZ | ✅ | `ktx create --encode basis-lz` / `toktx --encode etc1s` |
| **BC1–BC7** | ❌ 工具链内无 BC 编码器（libktx 只带 `astc_encode.cpp` 与 `basis_encode.cpp`） | ① 先编 UASTC/BasisLZ，再 `ktx transcode --target bcN`；② 外部 BC 编码器（DirectXTex `texconv` 等）出裸块数据，再 `ktx create --raw` |
| ETC2 | ❌ | 只能 `ktx transcode --target etc-rgba / eac-*` |
| BC6H（HDR） | ❌ | 本工具链无 HDR→BC6H 路径 |
| 16bit / HDR（如 `R16G16B16A16_SFLOAT`） | ⚠️ 未验证 | 需输入本身就是 16bit 数据类型，工具会校验：8bit PNG 报 `Input file data type "png_rgb" does not match the expected input data type of 16 bit "SFLOAT"` |

`transcode` 的目标格式清单：`etc-rgb` `etc-rgba` `eac-r11` `eac-rg11` `bc1` `bc3` `bc4` `bc5` `bc7` `astc`（固定 4x4）`r8` `rg8` `rgb8` `rgba8`。

## 4. 常用命令

> 一句话记住：**这批工具能直接编 ASTC 和 Basis（UASTC/ETC1S），编不了 BC**。想要 BC，只能"先编 UASTC，再 `transcode` 成 BC"，或者用外部 BC 编码器出裸数据再 `--raw` 打包。
> `transcode` 不是格式改名，是真的把 UASTC 的每个块解码后重新写成 BC7 块，所以中间那次 UASTC 有损会带进结果里。

### 4.1 图片 → ASTC（带 mipmap，移动端）

```powershell
tools\ktx\ktx.exe create --format ASTC_6x6_SRGB_BLOCK --astc-quality medium --generate-mipmap in.png out.ktx2
```

toktx 等价写法（块尺寸走 `--astc_blk_d`，不写格式名；sRGB 由图片自身推断）：

```powershell
tools\ktx\toktx.exe --t2 --encode astc --astc_blk_d 6x6 --astc_quality thorough --genmipmap out.ktx2 in.png
```

`--astc-quality`：`fastest | fast | medium | thorough | exhaustive`（对应质量 0/10/60/98/100）。法线图加 `--normal-mode`（ASTC 会用法线调优的参数集）。

### 4.2 图片 → BC7（带 mipmap，PC）

两条命令，第二步只是把 UASTC 解码成 BC7 写回 KTX2，mip 链原样保留：

```powershell
tools\ktx\ktx.exe create  --format R8G8B8A8_SRGB --encode uastc --uastc-quality 2 --uastc-rdo --generate-mipmap in.png tmp.ktx2
tools\ktx\ktx.exe transcode --target bc7 tmp.ktx2 out.ktx2
```

- 源是 sRGB 时结果 vkFormat 为 `BC7_SRGB_BLOCK`，源为 linear 时为 `BC7_UNORM_BLOCK`。
- 无 alpha 用 `--target bc1`；带 alpha 想要 4bpp 省空间用 `--target bc3`；法线用 `--target bc5`；单通道遮罩用 `--target bc4`。
- 法线链：第一步改用 `--format R8G8B8A8_UNORM --assign-oetf linear --normal-mode`，产物为 `BC5_UNORM_BLOCK`（transfer LINEAR，实测校验通过）。`--normal-mode` 会把 XYZ 法线转成 XY+alpha 再编码，shader 侧需按 `nml.xy = tex.ga*2-1; nml.z = sqrt(1-dot(nml.xy,nml.xy))` 还原。
- 质量注意：这条链经过了 UASTC 有损编码，BC7 结果是"UASTC 解码值的再量化"，不如原生 BC7 编码器（如需更高画质用 §4.3）。

### 4.3 外部 BC 编码器 → ktx2（画质最高，需额外工具）

本机目前未装 `texconv` / `magick` / `astcenc` / `ffmpeg`，走这条路要先弄一个原生 BC 编码器（推荐 DirectXTex 的 `texconv`）。`ktx create --raw` 只接受**裸块数据**，所以要先把 DDS 头剥掉：

```powershell
texconv -f BC7_UNORM -dx10 -o dds out.png
pwsh -c "$b=[System.IO.File]::ReadAllBytes('dds\out.dds'); [System.IO.File]::WriteAllBytes('bc7.raw',$b[148..($b.Length-1)])"
tools\ktx\ktx.exe create --raw --format BC7_SRGB_BLOCK --width 1024 --height 1024 bc7.raw out.ktx2
```

- 数据起点偏移：带 `-dx10`（BC6H/BC7 必须）= 148 字节（4 magic + 124 header + 20 DX10 header）；DX9 头的 BC1/BC3/BC4/BC5 = 128 字节。
- 带 mip 链时按 level 顺序（base 先）把每一级的裸数据都传进来，并加 `--levels N`；多层/立方体的顺序为 layer → face → depth → level。
- `--raw` 必须给 `--width`（否则报 `Option --width is missing`）。
- `--raw` 不校验数据，尺寸/格式给错也能写出文件，务必补一次 `ktx validate`。

### 4.4 只在已有 KTX2 上做二次处理

```powershell
tools\ktx\ktxsc.exe --zcmp 19 -o out_sc.ktx2 -f in.ktx2
tools\ktx\ktxsc.exe --encode astc --astc_blk_d 6x6 -o out_astc.ktx2 -f in_uncompressed.ktx2
tools\ktx\ktx2ktx2.exe -f in.ktx
```

`ktxsc` 实测：1200KB 的未压缩 KTX2 → zstd 后 220KB；→ ASTC 6x6 后 134KB。zstd 对未压缩与 UASTC 有效，ETC1S 本身就是超压缩格式（不能再叠 zstd）。

### 4.5 打包时常用参数

| 需求 | `ktx create` | `toktx` |
| --- | --- | --- |
| 立方体贴图（6 张，顺序 +X -X +Y -Y +Z -Z） | `--cubemap` | `--cubemap` |
| 纹理数组 | `--layers N` | `--layers N` |
| 3D 纹理 | `--depth N` | `--depth N` |
| 通道重排（如灰度写到 RGB：`rrr1`） | `--input-swizzle rrr1` | `--input_swizzle rrr1` / `--target_type R` |
| 强制色彩空间 | `--assign-oetf srgb` | `--assign_oetf srgb` |
| 多线程 | `--threads N` | `--threads N` |
| 未压缩数据叠 zstd | `--zstd 19` | `--zcmp 19` |

## 5. mipmap：开关与生成方式

| 模式 | 参数 | 产物 | 适用 |
| --- | --- | --- | --- |
| 不要 mipmap | 不加参数 | `levelCount = 1` | UI 图标等 |
| **烘进文件** | `ktx create --generate-mipmap`（可选 `--mipmap-filter lanczos4`、`--mipmap-filter-scale`、`--mipmap-wrap clamp`、`--levels N` 只生成前 N 级）；`toktx --genmipmap`（可选 `--filter/--fscale/--wmode`） | `levelCount = N` | **所有压缩贴图只能用这个** |
| 运行时生成 | `ktx create --runtime-mipmap`；`toktx --automipmap` | `levelCount = 0` | 仅未压缩格式，且需加载端支持运行时生成 |
| 手工给每级图 | `create --levels N` + 多个输入；`toktx --mipmap`（+ `--levels`） | 按给的图 | 需要自定义 mip 过滤时 |

坑：`--runtime-mipmap` / `--automipmap` 用在压缩贴图上会产出**非法文件**（`ktx validate` 报 `error-3017: levelCount cannot be 0 for block-compressed formats`），而 `ktx create` 本身不会拦你。压缩贴图务必 `--generate-mipmap` / `--genmipmap`。

非 2 次幂尺寸同样能生成完整 mip 链（640×480 → 10 级）。块压缩尺寸不整除块时按整块向上取整存储（640×480 用 ASTC 6x6 → 107×80 块 = 136960 字节），建议尺寸取块尺寸的整数倍。

## 6. 校验与查看

```powershell
tools\ktx\ktx.exe info out.ktx2
tools\ktx\ktx.exe info -f json out.ktx2
tools\ktx\ktx.exe validate out.ktx2
tools\ktx\ktx.exe extract --level 0 out.ktx2 check.png
```

`ktx info` 会隐式做 `validate` 并打印错误/警告；`-f mini-json` 适合脚本取 `header.vkFormat / levelCount / pixelWidth` 等字段。`ktx2check` 只做结构检查，上面那种 `levelCount=0` 的非法压缩文件它照样返回 0，**不要拿它当质量闸门**，用 `ktx validate`。

BC / ASTC 文件无法 `extract` 成 PNG（`Requested format conversion from VK_FORMAT_BC7_SRGB_BLOCK is not supported`），只能 `extract --raw --all` 出裸块数据。想肉眼看画质，保留中间的 UASTC 文件并 `extract --transcode rgba8`，或直接对源 PNG 与 `transcode --target rgba8` 的结果做对比。

## 7. 实测体积与耗时

1024×1024 sRGB 图 + 完整 mip 链，本机多核，`ktx create`（`--generate-mipmap`），耗时含写盘：

| 产物 | 格式 | 体积 | 耗时 |
| --- | --- | --- | --- |
| 未压缩（基准） | `R8G8B8A8_SRGB` | 5461 KB | 0.1 s |
| ASTC 6x6 medium | `ASTC_6x6_SRGB_BLOCK` | 612 KB | 0.1 s |
| ASTC 6x6 thorough | `ASTC_6x6_SRGB_BLOCK` | 612 KB | 0.4 s |
| ASTC 4x4 thorough | `ASTC_4x4_SRGB_BLOCK` | 1366 KB | 0.4 s |
| UASTC + RDO + zstd19 | `VK_FORMAT_UNDEFINED` | 690 KB | 2.1 s |
| ETC1S qlevel128 | `VK_FORMAT_UNDEFINED` | 143 KB | 0.5 s |
| UASTC → BC7（transcode） | `BC7_SRGB_BLOCK` | 1366 KB | 0.1 s |
| UASTC → BC3 / BC5 | 8 bpp | 1366 KB | 0.1 s |
| UASTC → BC1 / BC4 | 4 bpp | 683 KB | 0.1 s |

BC7/ASTC4x4/UASTC 都在 8bpp 量级，差别在画质与是否可再转码；ETC1S 体积最小但画质与转码目标最受限。ASTC/UASTC/ETC1S 的 bpp 参考表见 `toktx --help`（ASTC 4x4=8bpp … 12x12=0.89bpp）。

## 8. 坑清单

1. BMP/TGA 不认，先转 PNG（§1）。
2. 带非 sRGB ICC/自定义 gamma 的 PNG：`toktx` 直接失败并提示 `Its encoding gamma ... is not automatically supported by KTX`，需加 `--assign_oetf srgb`（信任文件是 sRGB）或 `--convert_oetf srgb`（做真实转换）；`ktx create` 能读，但要自己选对 `*_SRGB_*` 格式名，必要时 `--assign-oetf`。
3. 块压缩格式名漏 `_BLOCK` → `The requested format is invalid or unsupported`。
4. `ktx create --encode` 时 `--format` 描述的是**输入**格式，只能是 8bit 未压缩那几种；目标格式由 `--encode` 决定。ASTC 是唯一例外：目标格式直接写进 `--format`。
5. `toktx --levels N` 必须配合 `--mipmap`（否则 `too few input files`）；想生成"完整链但只保留前 N 级"用 `ktx create --generate-mipmap --levels N`。
6. 参数顺序相反：`ktx create [opts] <输入...> <输出>`，`toktx [opts] <输出> <输入...>`。
7. 压缩贴图禁用 `--runtime-mipmap` / `--automipmap`（§5）。
8. `transcode` 只能从可转码文件（UASTC / BasisLZ，vkFormat 为 `VK_FORMAT_UNDEFINED`）出发；ASTC、BC 文件再转报 `KTX file is not transcodable`。
9. `--generate-mipmap` 与 `--runtime-mipmap` 互斥；`--generate-mipmap` 不能用于 UINT 与 3D 纹理。
10. 选项与文件名易混时用 ` -- ` 分隔（如 `toktx --zcmp 3 -- out.ktx2 in.png`）。

## 9. 引擎侧现状（KTX2 已完整支持）

loader 在 `src/pulse_graphics/src/loader/load_texture.cpp`，注册扩展名 `ktx,ktx2`（按文件头识别，与扩展名无关）。支持矩阵：

| 资产 | 引擎行为 |
| --- | --- |
| 未压缩 RGBA8（`R8G8B8A8_UNORM/SRGB`，含 `--zstd N` 超压缩） | 直接上传；文件的 `levelCount=0`（`--runtime-mipmap`，未压缩且非 cubemap）在加载时由引擎生成 mip 链（走 render-graph 的 generate-mip pass，纹理加 `RENDER_TARGET`） |
| UASTC / UASTC+zstd / ETC1S | 运行时转码为设备支持的最优目标：**BC7 → BC1/BC3（按有无 alpha）→ ASTC 4x4 → ETC2 → RGBA32**，按 DFD transfer 自动选 UNORM/SRGB 变体；RGBA32 是无条件兜底（跳过能力过滤），保证任何设备可加载并打 32bpp warning |
| 原生 BC1–BC7 / ETC2 / EAC / ASTC | 查 `format_supports` 的 SAMPLE 位后直接上传；**不支持时显式失败，原生块格式没有运行时兜底**（引擎不链接 BC/ASTC 解码器）。报错打 vkFormat 名 + 尺寸，并提示**从该贴图的 UASTC/ETC1S 母文件重新 `ktx transcode --target <format>`**（`transcode` 读不了已块压缩的文件，见 §8.8） |
| RGB8（`R8G8B8_UNORM/SRGB`） | 会映射上传，但 `R8G8B8` 不是 Vulkan 强制格式：设备（如 Intel 核显）缺 SAMPLE 位时**显式失败**（不是静默转 RGBA8）。资产要跨平台就用 RGBA8 |
| KTX1 文件 | 拒绝并提示用 `ktx create` 重打包或 `ktx2ktx2` 转换 |
| 纹理数组 / 3D（`numLayers>1` 或 `baseDepth>1`） | 显式失败（能力待做，见 `docs/v0.3/ktx2贴图加载设计.md`） |
| cubemap（`numFaces==6, numLayers==1`） | loader 侧接受：按 `array_size=6` + `TEXTURE_CUBE` 上传烘好的 mip 链。但仓库暂无 cubemap 测试资产，端到端渲染未经验证；且运行时生成对 cubemap 关闭（generate-mip pass 只支持单面，见上表 `--runtime-mipmap` 行） |
| 压缩贴图 / cubemap 的 `--runtime-mipmap`（levelCount=0） | 不报错，按文件里的单层上传并 warning 提示打包时 `--generate-mipmap` 烘链（libktx 加载路径不校验此非法组合，`error-3017` 只在离线 `ktx validate`） |

实测（130×130、8 级 mip、开 Vulkan 校验层）：UASTC→BC7 ≈ 40 ms/张，zstd UASTC ≈ 1 ms，ETC1S ≈ 0 ms；关校验层更快。转码发生在 loader 的 PROCESSING 步（主线程、每帧限额内）。日志：转码 `ktx2: <文件> <模型> <源vkFormat> -> <目标vkFormat>, <W>x<H> x<levels> levels, in <N> ms`；运行时生成 `ktx2: <文件> generates <N> mips from the stored level at upload`。

### 推荐打包命令

```powershell
# PC 通用（UASTC+RDO，运行时自动落 BC7，兼容一切设备）
tools\ktx\ktx.exe create --format R8G8B8A8_SRGB --encode uastc --uastc-quality 2 --uastc-rdo --generate-mipmap in.png out.ktx2
# PC 通用 + zstd 超压缩（体积约再省 20%）
tools\ktx\ktx.exe create --format R8G8B8A8_SRGB --encode uastc --uastc-quality 2 --uastc-rdo --zstd 19 --generate-mipmap in.png out.ktx2
# 极致的 PC 发行：预转 BC7（免运行时转码；换设备不兼容）
tools\ktx\ktx.exe transcode --target bc7 tmp_uastc.ktx2 out.ktx2
# 体积敏感（如大量小图标）：ETC1S，注意 --levels 截断
tools\ktx\ktx.exe create --format R8G8B8A8_SRGB --encode basis-lz --qlevel 128 --generate-mipmap --levels 8 in.png out.ktx2
```

### 引擎时代的坑（新增）

1. `--encode basis-lz --generate-mipmap` 生成完整 mip 链会产出**非法文件**（`error-4001: ... byteLength incorrectly ordered`），用 `--levels N` 截断最小 mip 到 ≥4×4 即合法；ETC1S 块为 8×8，建议链尾留在 16×16 附近。
2. `ktxsc --zcmp 19` 给**已有** UASTC 文件叠 zstd 会报 `error-7013: Duplicate key-value entry "KTXwriterScParams"`；zstd 要在首次 `ktx create --encode uastc --zstd 19` 时指定。
3. RGB8 系不要用作交付格式（见支持矩阵）；`tools/ktx` 早期文档中"23=R8G8B8 展开上传"的旧路径已删除。
4. 非 2 幂/非块对齐尺寸（如 130×130 BC7）现在可正常上传：staging 按块向上取整（`bufferRowLength`），`imageExtent` 用真实 texel 尺寸，引擎与 cgpu 已按 Vulkan 规则对齐，无需迁就 4/8 整除。

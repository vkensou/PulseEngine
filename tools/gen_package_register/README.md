# gen_package_register

根据模块 IDL 中的插件描述（plugin desc，如 `PulseWindowPluginDesc`）自动生成其
`pulse_package_register` 实现，输出到 `<module>/src/package_register.cpp`。

与 `tools/idl` 共享同一套 IDL 工具链（`codegen.lua` / `idl.lua`），生成的文件同样
**不允许手改**，修改 `.idl` 后重新运行本工具即可。

## 用法

```bat
rem 批量生成全部模块（generate.bat 逐模块调用 generate.lua）
generate.bat

rem 只生成单个模块
lua54 generate.lua ..\..\src\pulse_window\idl\pulse_window.idl
```

`generate.lua` 一次只处理一个模块（参数为模块 IDL 路径），批量处理由
`generate.bat` 在外部循环驱动。

## 生成逻辑

1. 从模块 IDL 中找出插件描述结构体 `struct.XxxPluginDesc` 与配套函数
   `func.AddXxxPlugin`、`func.XxxPluginDescDefault`。
2. **有 desc 的模块**（pulse_window / pulse_asset / pulse_graphics /
   pulse_imgui / pulse_daslang）：
   - 用 `pulse_xxx_plugin_desc_default()` 得到默认 desc；
   - 若传入 `config`，按字段类型从 `PulseConfig` 读取：
     `bool` → `pulse_config_get_bool`，`float/double` → `pulse_config_get_double`，
     整型 → `pulse_config_get_int`，`cstring` → `pulse_config_get_string`，
     flag/枚举 → `(EPulseX)(uint32_t)pulse_config_get_int`，
     嵌套值结构体 → `pulse_config_get_obj` 后递归映射；
   - 跳过 `struct_size` / `version`、指针、数组、函数指针等字段；
   - 调用 `pulse_add_xxx_plugin(app, &desc)`，并把
     `EPulseAppAddPluginResult` 完整映射到 `EPulseResult`。
3. **无 desc 的模块**（pulse_input / pulse_transform / pulse_renderer）：
   `config` 必须为 NULL，直接调用 `pulse_add_xxx_plugin(app)`。
4. 没有插件 Add 函数的模块（pulse_app / pulse_config /
   pulse_package_loader）自动跳过。

## 验证

生成后请确认各模块原手写的 `pulse_package_register` 已删除（避免重复符号），
然后重新构建对应 target：

```bat
xmake build pulse_window
```
# tablegen

数据表绑定生成器。读 `.schema` 文件，输出 C++ 绑定与 daslang 绑定，供 `pulse_datatable`
在运行时使用。生成逻辑与运行时的分工见 [数据表方案](../../docs/v0.3/数据表方案.md)。

工具直接链接 `pulse_datalist_static` 复用 datalist 解析器，不重新实现一遍 schema 解析。

## 用法

```bat
xmake build tablegen
tablegen [--out-h <header> --out-cpp <source>] [--out-das <module>.das] <schema文件>...
```

- `--out-h` / `--out-cpp`：C++ 绑定，成对出现。
- `--out-das`：daslang 绑定。模块名取自输出文件名（`pulse_tables.das` → `module pulse_tables`），
  入口脚本 `require pulse_tables public` 即可用。
- 两者至少要有一个。纯 daslang 包不生成 C++ 绑定，只写 `--out-das`。
- 参数里的全部 `.schema` 一起参与生成：类型互相引用（struct 列、enum 列、`ref` 列）必须一次性传入。
- C++ 侧只生成，不注册；注册由生成的 `RegisterSchemas(system)` 在运行时完成。daslang 侧的注册由生成的 `.das` 自己做。

重新生成仓库里的全部产物：

```bat
tools\tablegen\generate.bat
```

## 输入

一个 `.schema` 文件是一个类型，文件名即类型名。kind 自动判定：

| kind | 判定条件 | 生成物 |
|---|---|---|
| table | 恰好一个字段 `key: true` | `Pulse<名>Row` 行结构体 + `Pulse<名>RowTable` + schema 描述符 + `fill_row` |
| enum | 顶层只有一个 `values` 列表 | 字符串白名单类型，被列引用时内嵌进该表描述符的 `PulseDataTableEnumDesc` |
| struct | 其余 | `Pulse<名>` 结构体 + `PulseDataTableStructDesc` |

字段属性：`type`、`key`、`default`、`min` / `max`、`values`、`ref`。详细语义与数据文件格式见
[数据表方案](../../docs/v0.3/数据表方案.md) §2、§3。

## 输出

### C++

- 行结构体 `alignas(N)`，紧跟 `static_assert(sizeof(...) == N)`，layout 一旦与运行时算出的行步长
  不一致就编译失败。类型名是 schema 文件名的 PascalCase：`snake_config` → `PulseSnakeConfigRow`。
- 结构体按依赖顺序输出，指针列另有前置声明，因此表之间互相引用不依赖文件顺序（也不允许 struct 自嵌套，
  由校验拦下）。
- 每列一个 `PulseDataTableColumnDesc`（`pulse::datatable::ColumnDescBuilder` 链式构造，偏移用 `offsetof`），
  每张表一个列数组、一个 `PulseDataTableSchemaDesc`、一份 `fill_row`。
- `PulseDataTableSchemaDesc` 只放类型名、列数组与主键下标，行大小与对齐由运行时校验 `static_assert`
  的同一套规则确认；描述符里 `fill_row` 非空，所以引擎按生成代码自带的 `offsetof` 布局填充。
- 枚举列（`values` 白名单列、`type` 为 enum schema 的列）在行结构体里就是 `std::string_view`，
  存校验后的白名单名字；不生成 `EPulse*` 枚举常量。被引用的 enum schema 以 `PulseDataTableEnumDesc`
  内嵌进使用它的表描述符。
- `RegisterSchemas(PulseDataTableSystemId)` 注册全部 table schema，任一注册失败即返回错误码。
- `Pulse<名>RowTable`：`Load(app, path = nullptr)`（记住路径，后续调用复用；`path` 为空用 `DefaultPath()`）、
  `IsReady`、`GetError`、`Rows(app, out_count)`、`GetRow(app, key)`（string 主键 `const char*`，
  int 主键 `int64_t`）、`DefaultPath()`。

用法见 `tests/datatable/test_basic.cpp`。

### daslang

- 每张表一个私有注册函数 `Pulse<名>RegisterSchema(app) : bool`：用 das 的
  `array<PulseDataTableColumnDesc>`（外加 `array<PulseDataTableStructDesc>` / `array<PulseDataTableEnumDesc>`）
  按 schema 逐列摆描述符，指针字段用 `unsafe { desc.p_columns = reinterpret<void?>(addr(columns[0])) }`
  指到数组首元素，然后直接调用 `pulse_data_table_register_schema(app, desc)`。不需要内嵌 schema 文本，
  也不需要算偏移——引擎在 `fill_row` 为空时自己推导布局并拷贝进系统存储。
- `PulseTablesRegisterSchemas(app)` 惰性注册全部表、进程内一次性，失败的注册函数打印引擎给出的错误；
  每张表的 `Load` 先调它，游戏代码不需要注册入口。
- 每张表一组函数：`Pulse<名>Load(app, path = "<名>.datatable") -> PulseAssetRequest`、
  `Pulse<名>IsReady(app, request)`、`Pulse<名>GetError(app, request)`、`Pulse<名>RowCount/RowAt/FindRow(FindRowInt)`。
- 请求级查询显式收 `request`，没有隐藏状态：脚本把 `Load` 的返回值存进自己的资源/单例后一直用它。
  `GetError` 未失败时返回空串，可以直接当失败判定。
- 每个字段：`Pulse<名>Get<字段>`，嵌套 struct 逐层拼名（`Pulse<名>Get<字段><子字段>`），最多四层。
- 字符串列返回 das `string`（按值拷贝），枚举列返回名字字符串，`ref` 列返回目标行裸指针，
  可以继续用目标表的访问器读。

daslang 侧不镜像行内存布局，字段读取走 `pulse` 模块的通用接口，所以行结构体布局变化不会影响脚本。
daslang 包不需要任何 native 库：注册用的描述符全在 das 侧构造。

用法见 `tests/datatable/das/probe.das` 与 `examples/snake_daslang/`。

## 校验

生成前先校验，任一失败即报错退出（错误信息带 schema 路径与行号）：

- 未知类型、table 当列类型、struct 自嵌套。
- 缺主键 / 多主键 / 主键类型非 string、int / 主键带默认值或 `values`。
- `values` 用于非 string、`min` / `max` 用于非 int、float、`ref` 与 `type` 不匹配、`ref` 目标不是 table。
- 非法 `default`（int 列给小数、bool 列给非 true/false）；struct 列不能带 `default`；
  enum 列的 `default` 必须是该 enum 的白名单值。
- enum schema 没有 `values`、有空串或重复值；enum schema 与字段混写。
- 生成的 daslang 访问器重名（例如同表内 `skill_power` 与 `skill.power`），或字段名与表函数重名。

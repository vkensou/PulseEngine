# datalist 格式

datalist 来源于 [datalist](https://github.com/cloudwu/datalist)。是 PulseEngine 的文本数据结构格式,实现在 `src/pulse_datalist`。本质上是简化版的yaml。`.prefab`、`.material`、`.shader` 等资产都基于它。

---

## 1. 数据类型

一个节点要么是标量,要么是容器:

| 类型 | 含义 |
| --- | --- |
| `NIL` `BOOL` `INT` `DOUBLE` `STRING` | 标量 |
| `LIST` | 数组 |
| `MAP` | 字典 |
| `MIXED` | 两者都有;只由「同名 key 重复出现」产生(§10) |

### 1.1 标量

| 写法 | 例子 | 结果 |
| --- | --- | --- |
| 十进制整数 | `42` `-7` `+3` | `INT` |
| 十六进制整数 | `0x1F` | `INT`(31) |
| 浮点 | `1.5` `.5` `1.` `1e3` | `DOUBLE` |
| 十六进制浮点 | `0x1p+0` | `DOUBLE`(1.0) |
| 布尔 | `true` `false` | `BOOL` |
| 空 | `nil` | `NIL`,但作为值时等于「不写这个键」(§10) |
| 引号字符串 | `"hi"` `'hi'` | `STRING` |
| 其它裸 atom | `inf` `assets/Quad.obj` `Rel(Tgt)` `中文` | `STRING` |

- 关键字区分大小写:`True` 是字符串 `"True"`;
- 裸 atom 一直读到分隔符(空格 / `\t` / 换行 / `,` / `{` `}` `[` `]` / `$` / `:` / `"` `'`),所以路径、`a-b`、`Rel(Tgt)` 都不需要引号;
- 单独的 `+` 和 `.` 是字符串;单独的 `-`、`--`、`---` 是分节符(`x : -` 报 `Invalid atom`),`-5` 才是负整数。

---

### 1.2 字符串与转义

单双引号等价;字符串**不能跨行**,未闭合报 `Invalid token`。

| 转义 | 含义 |
| --- | --- |
| `\\` | 反斜杠 |
| `\'` `\"` | 引号 |
| `\n` `\r` `\t` `\a` `\b` `\v` | 控制字符 |
| `\0` | NUL |
| `\1` … `\255` | 十进制 ASCII(1–3 位,值 ≤ 255;`\65` 是 `A`) |
| `\x41` | 十六进制 ASCII(1–2 位) |
| `\` + 真实换行 | 续行(换行本身进字符串) |

其它转义报 `Invalid quote string`。

```
x : "hello\tworld"      # hello<TAB>world
y : "C:\\path\\a"       # C:\path\a
z : 'it\'s'             # it's
```

## 2. 语法

同时支持缩进形态和括号形态，两者可以任意嵌套混用。

### 2.1 缩进形态

`key :` 之后换行并加深缩进,值就是一个子节,子节是 map 还是 list 由内容决定:

```
x :
  1 2 3
y :
  dict : "hello world"
z : { foobar }
100 : number
```
得到：`x = [ 1,2,3 ], y = { dict = "hello world" }, z = { "foobar" }, ["100"] = "number"`

规则:

- 缩进宽度按字符算,**`\t` 记 4 个空格**,`\t` 与 4 空格可混用;
- 同层的键缩进必须相同;比上一行更深、又不在 `key :` 之后的缩进报 `Invalid ident`;
- 缩进回到更浅一层即结束当前节;空行与纯注释行不影响缩进;
- 一行可以写多个键值对:`a : 1 b : 2`;
- `key :` 后面必须有值,直接换行到同级或文件尾报 `Invalid atom`;
- 键必须是裸 atom(`"k" : 1` 报 `Invalid key`);数字可以当键(`100 : number`)。

---

### 2.2 括号形态

字典只能用`{}`括住，数组只能用`[]`括住。括号内换行与 `,` 随意(逗号只是分隔符,可省)：

```
{
  x : [1 2 3]
  y : {dict : "hello world"}
  z : { foobar }
  100 : number
}
```
得到：`x = [ 1,2,3 ], y = { dict = "hello world" }, z = { "foobar" }, ["100"] = "number"`

---

### 2.3 分节

用若干个`---`表示对象分节

```
---
x : hello
y : world
---
1 2 3
```

得到`[ { x = "hello", y = "world" }, { 1,2,3 } ]`

---

### 2.4 tag与引用

`&N` 给节点打标签(定义),`*N` 指向同一个节点(**不是拷贝**)。`N` 是十六进制(`&1`、`&badf00d` 都合法)。

例子：

```
--- &1   # This structure tagged by 1
"hello\nworld"
---
x : *1   # The value is the structure with tag 1
```
得到：`[ { "hello\nworld" } , { x = { "hello\nworld" } } ]`

| 位置 | 例子 | 结果 |
| --- | --- | --- |
| 分节头 | `--- &1` … `--- *1` | 两节是同一个节点 |
| 表 | `x : &1 { a : 1 }` + `y : *1` | `x` 与 `y` 是同一个节点 |
| 数组 | `x : &1 [ 1 2 ]` + `y : *1` | 同上,数组也能打 tag |
| 节内单独一行 | `---` 换行缩进 `&1` 换行内容 | 给该节打 tag |
| 分节标量 | `--- &1` 换行 `5` + `x : *1` | `x` 是 `5` |
| 前向引用 | `x : *1` 写在 `y : &1 …` 之前 | 先建占位节点,定义处填入 |
| 成环 | `--- &1` 换行 `x : *1` | 合法,`get_obj(a, "x") == a` |

- 只引用不定义报 `Unsolved tag 1`;同一个 tag 定义两次报 `Duplicate tag`;
- 引用是节点共享:**指针相等**就代表「两处是同一个对象」,prefab 的层级复用靠的就是这一点;
- 成环的树可以正常读取,但 `pulse_datalist_to_text` 会报 `Cycle in datalist tree` 返回 NULL;
- `&N` 后面必须跟括号,或行尾 + 更深缩进的节:`--- &1 x : 1` 报 `Invalid list`;
- `&N` 后面跟内联**标量**时占位节点不会被填入:`x : &1 5` 之后 `y : *1` 拿到的是空数组。给标量打 tag 要用分节形态。

---

### 2.5 `$converter`

`$name X` 等价于 `[ "name", X ]`,表示「用构造器 `name` 构造 `X`」。三个位置都成立:

```
p : $vec3 [ 1 2 3 ]      # p : ["vec3" [1 2 3]]
q : $obj { x : 1 }       # q : ["obj" {x : 1}]
```

```
r : $obj                 # r : ["obj" {a : 1}]
  a : 1
```

```
--- $obj                 # [ "obj", {x : 1} ]
x : 1
```

含义由消费方定义,`pulse_datalist` 只负责构出那个 2 元素数组;当前仓库里还没有消费方。

---

### 2.6 注释与空白

- `#` 到行尾;行首的 `#` 视为空行；
- 空格、`\t`、`,` 都是分隔符，`,`可省；
- 键值对之间不需要分隔符，`a : 1 b : 2`与换行写法等价。
- `=` **不是**分隔符：`x = 1` 不会报错,而是被当成 `"x" "=" 1` 三个元素的序列。键值对只能用 `:`。

---

### 2.7 同名 key 与 `nil`

**同名 key 重复出现**(值都是容器)会合并成 `MIXED`:第一份留在字典项里,后续每份挂一个元素到同一节点上。所以出现 N 次时 `pulse_datalist_object_count` 是 1、`pulse_datalist_count` 是 N-1。写回时展开成「同名 key 写多次」,可以重新解析回同样的形状。

```
m : { a : 1 }
m : { a : 2 }
```

→ `object_count == 1`、`count == 1`、`get_type == MIXED`、`get_obj(m)` 的值是 `{a : 1}`。

- 值是**标量**时报 `Multi-key (m) should be a table`;
- 重复的就是同一个节点(`m : *1`)时直接跳过,不会把节点挂到自己身上。

**`key : nil` 不产生这个键**(单次出现也不产生),等于「不写」:

```
a : nil
b : 1
```

→ `{ b : 1 }`(没有 `a`)。

- 表里 `x : { a : nil b : 1 }` → `{ b : 1 }`;
- 数组里 `x : [ nil 1 ]` → `[nil 1]`,`NIL` 是正常元素;
- 需要「空值占位」写 `{}` 或 `[]`。


### 3. 写回 API

| API | 行为 |
| --- | --- |
| `pulse_datalist_to_text(node, &len)` | 规范化输出:顶层 map 不写括号,`LIST` 出 `[ ]`,`MAP` / `MIXED` 出 `{ }`;字符串按需加引号与转义;浮点保证带小数位(`1e3` → `1000.0`) |
| `pulse_datalist_quote(str, len)` | 把任意字节串转成可写回的带引号形式(转义 NUL、控制字符、非 UTF-8 字节) |
| `pulse_datalist_free_string(str)` | 释放上面两个 API 返回的字符串 |

`to_text` 的输出可以重新解析回等价结构(类型、字符串内容都一致)。它是**规范化**输出:注释、缩进、`0x` 写法、引号风格、`$converter` 的书写形式都不保留——`$obj { x : 1 }` 写回后是 `["obj" {x : 1}]`。共享节点会就地展开(引用关系不保留),成环的树直接报错。

## 4. 读取 API

| API | 说明 |
| --- | --- |
| `create_from_text(text, len)` / `create_from_text_file(path)` | 解析;失败返回 NULL,原因看 `last_error` |
| `create_from_text_list(text, len)` | 列表模式:顶层 `k : v` 摊平成 `k v k v` 的数组(不建 map) |
| `get_type(node, key)` | 取类型;`key == nullptr` 表示节点自身 |
| `has(node, key)` | 是否有该键;`key == nullptr` 表示「节点本身是不是容器」 |
| `get_bool` / `get_int` / `get_double` / `get_string` | 按 key 取值,取不到返回传入的默认值;`key == nullptr` 表示节点自身 |
| `get_obj(node, key)` | 值是容器时返回子节点,标量返回 NULL |
| `count(node)` / `get(node, i)` | 元素个数 / 第 i 个元素 |
| `object_count` / `object_key` / `object_value` | 字典项个数 / 第 i 项的键 / 值 |
| `addref(node)` / `release(node)` | 引用计数;减到 0 时整棵树一起释放(所有节点同一个 arena) |

创建出来的节点引用计数是 1,用完 `release` 即可。

## 5. 报错对照

报错文本形如 `Line <行号> : <原因>`,`pulse_datalist_last_error()` 取最近一次。

| 报错 | 触发 |
| --- | --- |
| `Invalid atom` | 该位置放不了这个 token:`x :` 后没有值、`{` 里放 `&N`、`x : -` |
| `Invalid key` | 键不是裸 atom(`"k" : 1`) |
| `Need a :` | 键后面不是 `:` |
| `Invalid ident` | 缩进比上一行深,却又不在 `key :` 之后 |
| `Invalid close bracket` | 闭合符与打开的不匹配(`x : [ 1 2 }`) |
| `Invalid token` | 字符串未闭合或跨行 |
| `Invalid quote string` | 未知转义 |
| `A key value pair needs a table, use { } instead of [ ]` | `[ a : 1 ]` |
| `A table holds key value pairs, use [ ] for a list` | `{ 1 2 }`、`{ *1 }` |
| `Multi-key (X) should be a table` | 同名 key 重复且值是标量 |
| `Unsolved tag N` / `Duplicate tag` | 引用未定义 / tag 定义两次 |
| `not end` | 一个文件里写了多段顶层结构(如 map 后面又跟 `---`) |
| `too many layers` | 嵌套超过 256 层 |
| `Cycle in datalist tree` | 写回成环的树 |

---

## 6. 陷阱清单

1. 顶层不能「先 map 后分节」,分节形态要顶格以 `---` 开头。
2. `a : nil` 不产生键,不是「值为空」——空值占位用 `{}` / `[]`;
3. `&N` 后跟内联标量时 tag 绑的是空占位节点,标量打 tag 请用分节形态;
4. `=` 不是分隔符,`x = 1` 会被静默当成三个字符串;
5. 单独的 `-` / `--` / `---` 是分节符,不能当负号或字符串;
6. 同名 key 重复时,值是容器才合并,是标量直接报错;

options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require pulse/flecs_boost public

// 第二个脚本：与 das_test.das 一同被 loader 注入，但不是 manifest 的
// script_file，因此不会在 PostBuild 自动编译执行；测试在 PostBuild 之后
// 通过 pulse_load_module 从缓存同步编译它，验证「编译与加载分离」的
// 运行期语义（同步编译，调用返回后 marker 立即存在）。
struct DasSecondMarker {
    value: int
}

[export]
def importModule(var ctx: ModuleContext) {
    var cmd = CommandBuffer(world = ctx.world)
    let ent = cmd.entity()
    cmd |> set_entity_component(ent, DasSecondMarker(value = 456))

    print("daslang second module loaded.\n")
}
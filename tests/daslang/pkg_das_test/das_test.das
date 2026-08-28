options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require pulse/flecs_boost public

struct DasTestMarker {
    value: int
}

[export]
def importModule(var ctx: ModuleContext) {
    // 创建一个实体并写入一个标记组件，验证 das 端调用 pulse/flecs 绑定是通的。
    var cmd = CommandBuffer(world = ctx.world)
    let ent = cmd.entity()
    cmd |> set_entity_component(ent, DasTestMarker(value = 123))

    print("daslang test module loaded.\n")
}
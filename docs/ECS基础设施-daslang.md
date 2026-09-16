## ECS组件（Component）
没有特别的，就是普通的struct：
```
struct SnakeMove {
    interval : float
    lastTime : float
}
```

## ECS标签（Tag）
没有特别的，就是普通的无成员变量的struct：
```
struct IsApple {}
```

## ECS事件（Event）
没有特别的，就是普通的struct，Event后缀只是为了可读性：
```
struct AppleEatenEvent {
    apple : Entity
}
```

## ECS状态机
普通的枚举，但需要加state_machine标记初始状态：
```
[state_machine(init="UnInitialized")]
enum SnakeGameState {
    UnInitialized
    Loading
    LoadFailed
    Gaming
    GameOver
}
```

## ECS系统（System）
是普通函数，但利用daslang的宏系统做了大量封装。只有加了system标签的函数才是ECS系统，函数参数都有特殊含义：
```
[system(phase="update", state="UnInitialized|Loading")]
def loadSnakeResourcesSystem(app : PulseAppId, assets : Res<SnakeAssets>, state : SystemStateMachine<SnakeGameState>, cmd : CommandBuffer, primaryWindowQuery : Query<PulseWindow const, PulsePrimaryWindow const>) {
}
```
system标签有两个参数：phase表示调用阶段，目前支持的值有`init, update, postUpdate, render, imgui`；state表示什么状态下会启用该system，可以绑定多个state用"|"分割，省略表示所有状态都启用。

系统参数：

|类型名|说明|
|---|---|
|裸Component类型|主查询的组件类型可以直接写在系统参数上|
|Entity|主查询所属的Entity|
|PulseAppId|pulse engine的app|
|Res<Component>|引用ECS之外的资源|
|SystemStateMachine<State>|ECS状态机|
|CommandBuffer|延迟操作ECS World的接口|
|Query<Component1, Component2>|额外的ECS查询|
|SingletonQuery<Component1, Component2>|类似Query，但查询目标是全局唯一的|
|EventWriter<Event>|事件发送器|
|EventReader<Event>|事件接收器，每个系统最多一个事件接收器|
|ecs_iter_t? -const|系统遍历器，表示用户将手动查询，和上面所有的参数不能共存|

Pulse Engine内置的资源、组件等见： [pulse_components](src/pulse_daslang/das/pulse/pulse_components.das)

## 手动注册
现在没有实现自动注册系统，所以需要手动注册系统、资源和状态机。组件、标签、事件是自动注册的。同phase的系统注册顺序就是执行顺序。
```
[export]
def importModule(var ctx : ModuleContext) {
    state_machine_init(ctx.world, ctx.updatePipeline, type<SnakeGameState>, SnakeGameState.UnInitialized)
    register_resource(ctx.world, type<SnakeAssets>)

    ctx |> register_system(@@loadSnakeResourcesSystem)
    ctx |> register_system(@@handleSnakeInputSystem)
    ctx |> register_system(@@scheduleSnakeMoveSystem)
    ctx |> register_system(@@executeSnakeMoveSystem)
    ctx |> register_system(@@syncSnakeBodyPositionSystem)
    ctx |> register_system(@@eatAppleSystem)
    ctx |> register_system(@@increaseScoreSystem)
    ctx |> register_system(@@spawnAppleSystem)
    ctx |> register_system(@@onGameOverSystem)
    ctx |> register_system(@@snakeUISystem)
    ctx |> register_system(@@snakeFpsUISystem)
    ctx |> register_system(@@restartSystem)
}
```
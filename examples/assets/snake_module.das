options gen2

require flecs_systems
require pulse_ecs

struct Velocity {
    X: float
    Y: float
    Z: float
}

var private run_probe_batches = 0
var private move_probe_hits = 0
var private spawned_entity <- default<Entity>

[system(phase="update")]
def run_probe(var iter: Iter?) {
    unsafe {
        var it = *iter
        while (iter_next(it)) {
            run_probe_batches += 1
            if (run_probe_batches == 1) {
                print("spawned_entity_id=")
                dump_entity(spawned_entity)
                print("run_probe batch={it.count} tick={run_probe_batches}\n")
            }
        }
    }
}

[system(phase="update")]
def move_probe(e: Entity; pos: Position const; var vel: Velocity) {
    move_probe_hits += 1
    vel.X += 1.0
    if (move_probe_hits == 1) {
        print("move_probe entity_id=")
        dump_entity(e)
        print("move_probe pos={pos.value.X} vel={vel.X} {vel.Y} {vel.Z}\n")
    }
}

[export]
def importModule(var ctx:ModuleContext) {
    run_probe_batches = 0
    move_probe_hits = 0
    spawned_entity = default<Entity>

    var ent = create_entity(ctx.world)
    ecs_entity_set(ctx.world, ent, Position(value=make_vec3(10.0, 20.0, 30.0)))
    ecs_entity_set(ctx.world, ent, Velocity(X=1.0, Y=2.0, Z=3.0))
    spawned_entity = ent

    run_probe`register(ctx)
    move_probe`register(ctx)
}

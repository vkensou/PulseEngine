options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

module flecs_state_machine

require flecs public
require flecs_base public
require daslib/typemacro_boost
require daslib/class_boost
require daslib/ast_boost

// ============================================================
// Compile-time lookup
//
// [system(state="A|B")] is applied while the game module is being compiled,
// so we walk that module's enumerations with daslang's native
// for_each_enumeration and find the enum annotated with [state_machine].
// ============================================================

def state_machine_find_enum(mod: Module?) : EnumerationPtr {
    var result : EnumerationPtr
    if (mod == null) {
        return null
    }
    for_each_enumeration(mod) $(enu) {
        for (ann in enu.annotations) {
            if (ann.annotation != null && ann.annotation.name == "state_machine") {
                result = enu
            }
        }
    }
    return result
}

// [state_machine(init="UnInitialized")] marks the module's state enum.
// Systems annotated with [system(state="A|B")] are enabled only while the
// state machine is in one of the listed states.
[enumeration_macro(name="state_machine")]
class StateMachineEnumerationMacro : AstEnumerationAnnotation {
    def override apply(var enu : EnumerationPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        let init_arg = find_arg(args, "init")
        if (!(init_arg is tString)) {
            errors := "[state_machine] expects a string init argument, e.g. [state_machine(init=\"UnInitialized\")]"
            return false
        }

        let init_name = init_arg as tString
        var found = false
        for (entry in enu.list) {
            if (entry.name == init_name) {
                found = true
            }
        }
        if (!found) {
            errors := "state machine initial state '{init_name}' not found in enum {enu.name}"
            return false
        }
        return true
    }
}

// ============================================================
// Runtime storage (pure daslang)
//
// State machines are type-erased into a module global array. The state value
// is stored on the SingleHolder singleton uint64; the manager system runs at
// the beginning of the update phase and enables/disables registered systems
// and observers when the state changes.
// ============================================================

struct StateMachineEntry {
    target : uint64
    states : array<int64>
}

struct StateMachineStorage {
    component_id : uint64
    component_size : int
    applied : int64
    entries : array<StateMachineEntry>
}

var state_machine_storages : array<StateMachineStorage>
var state_machine_tick_registered = false

def state_machine_storage_index(component_id : uint64) : int {
    for (i in 0..length(state_machine_storages)) {
        if (state_machine_storages[i].component_id == component_id) {
            return i
        }
    }
    return -1
}

def state_machine_read_state(var world: World; component_id : uint64; size : int) : int64 {
    let ptr = get_component(world, get_single_holder(world), component_id)
    if (ptr == null) {
        return 0l
    }
    unsafe {
        if (size == 1) {
            return int64(*reinterpret<int8?>(ptr))
        } elif (size == 2) {
            return int64(*reinterpret<int16?>(ptr))
        } elif (size == 8) {
            return *reinterpret<int64?>(ptr)
        } else {
            return int64(*reinterpret<int?>(ptr))
        }
    }
}

def state_machine_tick_all(var world: World) {
    for (i in 0..length(state_machine_storages)) {
        let current = int64(state_machine_read_state(world, state_machine_storages[i].component_id, state_machine_storages[i].component_size))
        if (current != state_machine_storages[i].applied) {
            state_machine_storages[i].applied = current
            for (entry in state_machine_storages[i].entries) {
                var enabled = false
                for (state in entry.states) {
                    if (state == current) {
                        enabled = true
                    }
                }
                ecs_enable(world, entry.target, enabled)
            }
        }
    }
}

def stateMachineTickWrapper(var iter : ecs_iter_t?) : void {
    var world = get_world(iter)
    state_machine_tick_all(world)
}

def state_machine_ensure_tick(var world: World; update_pipeline : uint64) {
    if (state_machine_tick_registered) {
        return
    }
    state_machine_tick_registered = true

    var desc = SystemDesc(name = "DaslangStateMachine", dependsOn = update_pipeline, immediate = true)
    register_system_from_desc(world, desc, @@stateMachineTickWrapper)
}

// System parameter accessor for the state machine (PULSE_ECS_STATE_MACHINE).
// The state is stored as a component on the SingleHolder singleton uint64.
[template_structure(T)]
struct template SystemStateMachine {
    @safe_when_uninitialized world : World

    [class_method]
    def static const current() : T {
        unsafe {
            let ptr = ecs_entity_get(world, get_single_holder(world), type<T>)
            return *ptr
        }
    }

    [class_method]
    def static const is_state(state: T) : bool {
        return current() == state
    }

    [class_method]
    def static const to(next: T) : void {
        ecs_entity_set(world, get_single_holder(world), next)
    }
}

// Registers the state machine storage and installs the per-frame manager
// system which applies enable/disable state changes. Call this from
// importModule before registering any state annotated system.
def state_machine_init(var world: World; update_pipeline : uint64; t: type<auto(T)>; initial: T) {
    ecs_component(world, type<T>)
    ecs_entity_set(world, get_single_holder(world), initial)

    let component_id = ecs_component(world, type<T>)
    let index = state_machine_storage_index(component_id)
    if (index < 0) {
        var storage = StateMachineStorage(
            component_id = component_id,
            component_size = typeinfo sizeof(type<T>),
            applied = int64(initial))
        state_machine_storages |> emplace(storage)
        state_machine_ensure_tick(world, update_pipeline)
    } else {
        state_machine_storages[index].applied = int64(initial)
    }
}

// Registers one allowed state for a system/observer uint64. Generated by the
// [system] annotation for each state in the state="A|B" list.
def state_machine_reg_state(var world: World; t: type<auto(T)>; target: uint64; state: int64) {
    let component_id = ecs_component(world, type<T>)
    let index = state_machine_storage_index(component_id)
    if (index < 0) {
        return
    }

    let entry_count = length(state_machine_storages[index].entries)
    if (entry_count == 0 || state_machine_storages[index].entries[entry_count - 1].target != target) {
        var entry = StateMachineEntry(target = target)
        state_machine_storages[index].entries |> emplace(entry)
    }

    let new_entry_index = length(state_machine_storages[index].entries) - 1
    state_machine_storages[index].entries[new_entry_index].states |> push(state)

    // Apply immediately so systems registered after state_machine_init start
    // with the correct enable/disable state.
    let applied = state_machine_storages[index].applied
    var enabled = false
    for (value in state_machine_storages[index].entries[new_entry_index].states) {
        if (value == applied) {
            enabled = true
        }
    }
    ecs_enable(world, target, enabled)
}

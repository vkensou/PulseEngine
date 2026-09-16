options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

module pulse_components public

// Daslang-side layouts of PulseEngine C components.
// Field order must match the generated C headers so that ecs_component<T>
// resolves to the same flecs component id and reinterprets the storage.
require pulse public
require pulse/flecs_boost public

// ECS Res
struct PulseTimer {
    delta_time: float
    time_since_startup: float
    delta_time_double: double
    time_since_startup_double: double
    fps: int
}

// ECS Res
struct PulseKeyboardInput {
    pressed: bool[512]
    just_pressed: bool[512]
    just_released: bool[512]
}

// ECS Component
struct PulseWindow {
    struct_size: uint
    title: string
    width: int
    height: int
    resizable: bool
    external_graphics_context: bool
}

// ECS Tag
struct PulsePrimaryWindow {
}

// ECS Component
struct PulseLocalTransform {
    translation: HMM_Vec3
    rotation: HMM_Quat
    scale: HMM_Vec3
}

// ECS Component
struct PulseCamera {
    window_entity: Entity
    fov: float
    near_plane: float
    far_plane: float
}

// ECS Component
struct PulseRenderable {
    mesh: PulseMeshHandle
    material: PulseMaterialHandle
}

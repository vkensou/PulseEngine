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
require strings

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
    orthographic_size: float
    orthographic: bool
    clear_color: uint
}

// ECS Component
struct PulseRenderable {
    mesh: PulseMeshHandle
    material: PulseMaterialHandle
    sorting_order: int
}

struct PulseText {
    desc: PulseTextBlockDesc
    text: uint8[256]
    box_width: float
    box_height: float
    scissor_x: float
    scissor_y: float
    scissor_width: float
    scissor_height: float
    sorting_order: int
    revision: uint
}

def pulse_text_set_text(var text: PulseText, content: string) {
    let total = length(content)
    var count = total > 255 ? 255 : total
    peek_data(content) $(bytes) {
        if (count < total) {
            while (count > 0 && (uint(bytes[count]) & 0xC0u) == 0x80u) {
                count -= 1
            }
        }
        for (i in 0..count) {
            text.text[i] = bytes[i]
        }
    }
    text.text[count] = 0u8
}

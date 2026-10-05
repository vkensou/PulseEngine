#include "text_internal.h"

namespace pulse_text_internal {

void register_components(ecs_world_t* world) {
    {
        flecs::component<PulseTextColor> comp(world, "PulseTextColor");
        comp.member("r", &PulseTextColor::r);
        comp.member("g", &PulseTextColor::g);
        comp.member("b", &PulseTextColor::b);
        comp.member("a", &PulseTextColor::a);
    }
    {
        flecs::component<PulseTextBlockDesc> comp(world, "PulseTextBlockDesc");
        comp.member("chain", &PulseTextBlockDesc::chain);
        comp.member("size", &PulseTextBlockDesc::size);
        comp.member("color", &PulseTextBlockDesc::color);
        comp.member("align_h", &PulseTextBlockDesc::align_h);
        comp.member("align_v", &PulseTextBlockDesc::align_v);
        comp.member("line_height", &PulseTextBlockDesc::line_height);
        comp.member("auto_size", &PulseTextBlockDesc::auto_size);
    }
}

}

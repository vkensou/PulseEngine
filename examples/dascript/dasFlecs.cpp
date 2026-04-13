#include "dasFlecs.h"
#include "module_importer.h"
#include "predefine_components.h"

MAKE_TYPE_FACTORY(World, World);
struct WorldAnnotation final : das::ManagedStructureAnnotation<World>
{
	WorldAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("World", ml, "World")
	{
	}
};

MAKE_TYPE_FACTORY(Entity, Entity);
struct EntityAnnotation final : das::ManagedValueAnnotation<Entity>
{
	EntityAnnotation(das::ModuleLibrary& ml)
		: ManagedValueAnnotation(ml, "Entity", "Entity")
	{
	}
};

namespace das
{
	template <>
	struct cast<Entity> {
		static __forceinline Entity to(vec4f x) { return Entity{ cast<int>::to(x) }; }
		static __forceinline vec4f from(Entity x) { return cast<int>::from(x.b); }
	};
	template <> struct WrapType<Entity> { enum { value = true }; typedef int type; typedef int rettype; };
}

Entity create_entity(World& world)
{
	auto id_ = world.a++;
	Entity entity = { id_ };
	return entity;
}

void dump_world(const World& world)
{
	printf("%d\n", world.a);
}

void dump_entity(Entity entity)
{
	printf("%d\n", entity.b);
}

inline ModuleFlecs::ModuleFlecs() : Module("flecs")
{
	using namespace das;

	ModuleLibrary lib(this);
	lib.addBuiltInModule();
	addBuiltinDependency(lib, Module::require("math"));

	addAnnotation(make_smart<WorldAnnotation>(lib));
	addAnnotation(make_smart<EntityAnnotation>(lib));

	addExtern<DAS_BIND_FUN(create_entity)>(*this, lib, "create_entity", SideEffects::worstDefault, "create_entity")->args({ "world" });
	addExtern<DAS_BIND_FUN(dump_world)>(*this, lib, "dump_world", SideEffects::modifyExternal, "dump_world")->args({ "world" });
	addExtern<DAS_BIND_FUN(dump_entity)>(*this, lib, "dump_entity", SideEffects::modifyExternal, "dump_entity")->args({ "entity" });
}

REGISTER_MODULE(ModuleFlecs);

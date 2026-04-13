#include "dasFlecs.h"

#include "daScript/daScript.h"

namespace dasPulseECS
{
	Entity create_entity(World& world)
	{
		ecs_entity_desc_t desc = {};
		auto id_ = ecs_entity_init(world.world_, &desc);
		Entity entity = { id_ };
		return entity;
	}

	void dump_world(const World& world)
	{
		auto info = ecs_get_world_info(world.world_);
		printf("%lld\n", info->cmd.add_count);
	}

	void dump_entity(Entity entity)
	{
		printf("%lld\n", entity.entity_);
	}

	Query build_query(const World& world, const char* expr)
	{
		ecs_query_desc_t desc = { .expr = expr };
		ecs_query_t* query = ecs_query_init(world.world_, &desc);
		return Query{ query };
	}
}

MAKE_TYPE_FACTORY(World, dasPulseECS::World);
struct WorldAnnotation final : das::ManagedValueAnnotation<dasPulseECS::World>
{
	WorldAnnotation(das::ModuleLibrary& ml)
		: ManagedValueAnnotation(ml, "World", "dasPulseECS::World")
	{
	}
};

MAKE_TYPE_FACTORY(Entity, dasPulseECS::Entity);
struct EntityAnnotation final : das::ManagedValueAnnotation<dasPulseECS::Entity>
{
	EntityAnnotation(das::ModuleLibrary& ml)
		: ManagedValueAnnotation(ml, "Entity", "dasPulseECS::Entity")
	{
	}
};

MAKE_TYPE_FACTORY(Query, dasPulseECS::Query);
struct QueryAnnotation final : das::ManagedValueAnnotation<dasPulseECS::Query>
{
	QueryAnnotation(das::ModuleLibrary& ml)
		: ManagedValueAnnotation(ml, "Query", "dasPulseECS::Query")
	{
	}
};

struct ModuleContextAnnotation final : das::ManagedStructureAnnotation<dasPulseECS::ModuleContext>
{
	ModuleContextAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("ModuleContext", ml, "dasPulseECS::ModuleContext")
	{
		addField<DAS_BIND_MANAGED_FIELD(world)>("world");
	}
};

namespace das
{
	template <>
	struct cast<dasPulseECS::World> {
		static __forceinline dasPulseECS::World to(vec4f x) { return dasPulseECS::World{ cast<ecs_world_t*>::to(x) }; }
		static __forceinline vec4f from(dasPulseECS::World x) { return cast<ecs_world_t*>::from(x.world_); }
	};
	template <> struct WrapType<dasPulseECS::World> { enum { value = true }; typedef void* type; typedef void* rettype; };

	template <>
	struct cast<dasPulseECS::Entity> {
		static __forceinline dasPulseECS::Entity to(vec4f x) { return dasPulseECS::Entity{ cast<uint64_t>::to(x) }; }
		static __forceinline vec4f from(dasPulseECS::Entity x) { return cast<uint64_t>::from(x.entity_); }
	};
	template <> struct WrapType<dasPulseECS::Entity> { enum { value = true }; typedef uint64_t type; typedef uint64_t rettype; };

	template <>
	struct cast<dasPulseECS::Query> {
		static __forceinline dasPulseECS::Query to(vec4f x) { return dasPulseECS::Query{ cast<ecs_query_t*>::to(x) }; }
		static __forceinline vec4f from(dasPulseECS::Query x) { return cast<ecs_query_t*>::from(x.query_); }
	};
	template <> struct WrapType<dasPulseECS::Query> { enum { value = true }; typedef void* type; typedef void* rettype; };
}

class ModuleFlecs : public das::Module
{
public:
	ModuleFlecs() : Module("flecs")
	{
		using namespace das;

		ModuleLibrary lib(this);
		lib.addBuiltInModule();
		addBuiltinDependency(lib, Module::require("math"));

		addAnnotation(make_smart<WorldAnnotation>(lib));
		addAnnotation(make_smart<EntityAnnotation>(lib));
		addAnnotation(make_smart<QueryAnnotation>(lib));
		addAnnotation(make_smart<ModuleContextAnnotation>(lib));

		addExtern<DAS_BIND_FUN(dasPulseECS::create_entity)>(*this, lib, "create_entity", SideEffects::worstDefault, "create_entity")->args({ "world" });
		addExtern<DAS_BIND_FUN(dasPulseECS::dump_world)>(*this, lib, "dump_world", SideEffects::modifyExternal, "dump_world")->args({ "world" });
		addExtern<DAS_BIND_FUN(dasPulseECS::dump_entity)>(*this, lib, "dump_entity", SideEffects::modifyExternal, "dump_entity")->args({ "entity" });
		addExtern<DAS_BIND_FUN(dasPulseECS::build_query)>(*this, lib, "build_query", SideEffects::worstDefault, "build_query")->args({ "world", "query_desc" });
	}
};

REGISTER_MODULE(ModuleFlecs);

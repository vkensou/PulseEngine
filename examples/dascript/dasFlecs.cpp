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

	ecs_iter_t query_iter(const Query& query)
	{
		return ecs_query_iter(query.query_->world, query.query_);
	}

	bool query_next(ecs_iter_t& iter)
	{
		return ecs_query_next(&iter);
	}

	void* iter_field(ecs_iter_t& iter, int size, int index)
	{
		return ecs_field_w_size(&iter, size, index);
	}

	const char* get_pure_component_name(const char* component_name)
	{
		if (component_name == NULL) {
			return NULL;
		}

		const char* p = strrchr(component_name, ':');
		return (p == NULL) ? component_name : (p + 1);
	}

	ecs_id_t register_component(const World& world, const char* component_name, int size, int alignment)
	{
		ecs_entity_t ComponentID_ = 0; 
		{
			const char* pure_component_name = get_pure_component_name(component_name);
			ecs_component_desc_t desc = { 0 }; 
			ecs_entity_desc_t edesc = { 0 }; 
			edesc.id = ComponentID_; 
			edesc.use_low_id = true; 
			edesc.name = pure_component_name;
			edesc.symbol = pure_component_name;
			desc.entity = ecs_entity_init(world, &edesc); 
			desc.type.size = (static_cast<ecs_size_t>(size));
			desc.type.alignment = static_cast<int64_t>(alignment);
			ComponentID_ = ecs_component_init(world, &desc);
		} 
		if (!(ComponentID_ != 0)) {
			ecs_assert(ComponentID_ != 0, ECS_INVALID_PARAMETER, "failed to create component %s", component_name);
			return 0;
		} 
		return ComponentID_;
	}

	void set_component(World& world, Entity entity, ecs_id_t component_id, int size, const void* data)
	{
		ecs_set_id(world.world_, entity.entity_, component_id, size, data);
	}

	struct SystemCallBackContext
	{
		das::Func fn;
		das::Context* context;
		das::LineInfoArg* at;
	};

	void das_system_wrapper(ecs_iter_t* it)
	{
		SystemCallBackContext* callBackContext = (SystemCallBackContext*)it->run_ctx;
		das::das_invoke_function<ecs_iter_t*>::invoke(callBackContext->context, callBackContext->at, callBackContext->fn, it);
	}

	void das_system_context_free(void* ctx)
	{
		SystemCallBackContext* callBackContext = (SystemCallBackContext*)ctx;
		delete callBackContext;
	}

	void build_system_from_query_expr(const World& world, const char* name, Entity dependson, const char* query_expr, das::Func fn, das::Context* context, das::LineInfoArg* at)
	{
		ecs_id_t ids[] = { ecs_dependson(dependson.entity_), dependson.entity_, 0 };
		ecs_entity_desc_t edesc = { .name = name, .add = ids };
		ecs_entity_t entity = ecs_entity_init(world, &edesc);

		SystemCallBackContext* callBackContext = new SystemCallBackContext();
		callBackContext->fn = fn;
		callBackContext->context = context;
		callBackContext->at = at;

		ecs_system_desc_t sdesc = { 
			.entity = entity,
			.query = {.expr = query_expr},
			.callback = das_system_wrapper,
			.run_ctx = callBackContext,
			.run_ctx_free = das_system_context_free,
		};
		ecs_entity_t system = ecs_system_init(world, &sdesc);
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

MAKE_TYPE_FACTORY(Iter, ecs_iter_t);
struct IterAnnotation final : das::ManagedStructureAnnotation<ecs_iter_t>
{
	IterAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("Iter", ml, "ecs_iter_t")
	{
		addField<DAS_BIND_MANAGED_FIELD(count)>("count");
	}
};

struct ModuleContextAnnotation final : das::ManagedStructureAnnotation<dasPulseECS::ModuleContext>
{
	ModuleContextAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("ModuleContext", ml, "dasPulseECS::ModuleContext")
	{
		addField<DAS_BIND_MANAGED_FIELD(world)>("world");
		addField<DAS_BIND_MANAGED_FIELD(initPipeline)>("initPipeline");
		addField<DAS_BIND_MANAGED_FIELD(updatePipeline)>("updatePipeline");
		addField<DAS_BIND_MANAGED_FIELD(postUpdatePipeline)>("postUpdatePipeline");
		addField<DAS_BIND_MANAGED_FIELD(renderPipeline)>("renderPipeline");
		addField<DAS_BIND_MANAGED_FIELD(imguiPipeline)>("imguiPipeline");
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
		addAnnotation(make_smart<IterAnnotation>(lib));
		addAnnotation(make_smart<ModuleContextAnnotation>(lib));

		addExtern<DAS_BIND_FUN(dasPulseECS::create_entity)>(*this, lib, "create_entity", SideEffects::worstDefault, "create_entity")->args({ "world" });
		addExtern<DAS_BIND_FUN(dasPulseECS::dump_world)>(*this, lib, "dump_world", SideEffects::modifyExternal, "dump_world")->args({ "world" });
		addExtern<DAS_BIND_FUN(dasPulseECS::dump_entity)>(*this, lib, "dump_entity", SideEffects::modifyExternal, "dump_entity")->args({ "entity" });
		addExtern<DAS_BIND_FUN(dasPulseECS::build_query)>(*this, lib, "build_query", SideEffects::worstDefault, "build_query")->args({ "world", "query_desc" });
		addExtern<DAS_BIND_FUN(dasPulseECS::query_iter), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "query_iter", SideEffects::worstDefault, "query_iter")->args({ "query" });
		addExtern<DAS_BIND_FUN(dasPulseECS::query_next)>(*this, lib, "query_next", SideEffects::worstDefault, "query_next")->args({ "iter" });
		addExtern<DAS_BIND_FUN(dasPulseECS::iter_field)>(*this, lib, "iter_field", SideEffects::worstDefault, "iter_field")->args({ "iter", "size", "index" });
		addExtern<DAS_BIND_FUN(dasPulseECS::register_component)>(*this, lib, "register_component", SideEffects::worstDefault, "register_component")->args({ "world", "component_name", "size", "alignment" });
		addExtern<DAS_BIND_FUN(dasPulseECS::set_component)>(*this, lib, "set_component", SideEffects::worstDefault, "set_component")->args({ "world", "entity", "component_id", "size", "data" });
		addExtern<DAS_BIND_FUN(dasPulseECS::build_system_from_query_expr)>(*this, lib, "build_system_from_query_expr", SideEffects::worstDefault, "build_system_from_query_expr")->args({ "world", "name", "dependson", "query_expr", "fn", "context", "at"});
	}
};

REGISTER_MODULE(ModuleFlecs);

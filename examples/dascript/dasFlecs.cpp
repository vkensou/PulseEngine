#include "dasFlecs.h"

#include "daScript/daScript.h"
#include "ecsext.hpp"

namespace dasPulseECS
{
	Entity create_entity(World& world)
	{
		ecs_entity_desc_t desc = {};
		auto id_ = ecs_entity_init(world.world_, &desc);
		Entity entity = { id_ };
		return entity;
	}

	void destruct_entity(World& world, Entity entity)
	{
		ecs_delete(world.world_, entity.entity_);
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

	Entity get_single_holder(const World& world)
	{
		return flecs::_::type<pulse::SingleHolder>::id(world.world_);
	}

	Entity get_event_tag(const World& world)
	{
		return flecs::_::type<pulse::EventTag>::id(world.world_);
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

	bool iter_next(ecs_iter_t* iter)
	{
		return iter->next(iter);
	}

	void* iter_field(ecs_iter_t* iter, int size, int index)
	{
		return ecs_field_w_size(iter, size, index);
	}

	Entity iter_entity(ecs_iter_t* iter, int index)
	{
		ecs_assert(iter->entities != nullptr, ECS_INVALID_PARAMETER, "iterator has no entity array");
		ecs_assert(index >= 0 && index < iter->count, ECS_INVALID_PARAMETER, "iterator entity index %d out of range %d", index, iter->count);
		return Entity{ iter->entities[index] };
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
			desc.type.alignment = static_cast<int64_t>(size > 0 ? alignment : 0);
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

	const void* get_component(const World& world, Entity entity, ecs_id_t component_id)
	{
		return ecs_get_id(world.world_, entity.entity_, component_id);
	}

	const void* get_res(const World& world, ecs_id_t component_id)
	{
		return ecs_get_id(world.world_, component_id, component_id);
	}

	void* get_mut_res(const World& world, ecs_id_t component_id)
	{
		return ecs_get_mut_id(world.world_, component_id, component_id);
	}

	struct SystemCallBackContext
	{
		das::Func fn;
		das::Context* context;
		das::LineInfo at;
	};

	void das_system_wrapper(ecs_iter_t* it)
	{
		SystemCallBackContext* callBackContext = (SystemCallBackContext*)it->run_ctx;
		das::das_invoke_function<ecs_iter_t*>::invoke(callBackContext->context, &callBackContext->at, callBackContext->fn, it);
	}

	void das_system_context_free(void* ctx)
	{
		SystemCallBackContext* callBackContext = (SystemCallBackContext*)ctx;
		delete callBackContext;
	}

	void register_system_from_query_expr(const World& world, const char* name, Entity dependson, const char* query_expr, das::Func fn, das::Context* context, das::LineInfoArg* at)
	{
		if (!context) {
			return;
		}
		if (!fn) {
			context->throw_error_at(at, "register_system requires a valid callback function");
		}

		ecs_id_t ids[] = { ecs_dependson(dependson.entity_), dependson.entity_, 0 };
		ecs_entity_desc_t edesc = { .name = name, .add = dependson.entity_ != 0 ? ids : nullptr };
		ecs_entity_t entity = ecs_entity_init(world, &edesc);

		if (!entity) {
			context->throw_error_at(at, "failed to create flecs system entity '%s'", name ? name : "<unnamed>");
		}

		SystemCallBackContext* callBackContext = new SystemCallBackContext();
		callBackContext->fn = fn;
		callBackContext->context = context;
		if (at) {
			callBackContext->at = *at;
		}

		ecs_system_desc_t sdesc = {
			.entity = entity,
			.query = {.expr = query_expr},
			.run = das_system_wrapper,
			.run_ctx = callBackContext,
			.run_ctx_free = das_system_context_free,
		};
		ecs_entity_t system = ecs_system_init(world, &sdesc);
		if (!system) {
			ecs_delete(world, entity);
			context->throw_error_at(at, "failed to register flecs system '%s'", name ? name : "<unnamed>");
		}
	}

	void register_system_from_desc(const World& world, const SystemDesc& desc, das::Func fn, das::Context* context, das::LineInfoArg* at)
	{
		if (!context) {
			return;
		}
		if (!fn) {
			context->throw_error_at(at, "register_system requires a valid callback function");
		}

		ecs_id_t ids[] = { ecs_dependson(desc.dependsOn.entity_), desc.dependsOn.entity_, 0 };
		ecs_entity_desc_t edesc = { .name = desc.name, .add = desc.dependsOn.entity_ != 0 ? ids : nullptr };

		ecs_entity_t entity = ecs_entity_init(world, &edesc);
		if (!entity) {
			context->throw_error_at(at, "failed to create flecs system entity '%s'", desc.name ? desc.name : "<unnamed>");
		}

		SystemCallBackContext* callBackContext = new SystemCallBackContext();
		callBackContext->fn = fn;
		callBackContext->context = context;
		if (at) {
			callBackContext->at = *at;
		}

		ecs_system_desc_t sdesc = {
			.entity = entity,
			.query = {},
			.run = das_system_wrapper,
			.run_ctx = callBackContext,
			.run_ctx_free = das_system_context_free,
			.immediate = desc.immediate,
		};

		memcpy(sdesc.query.terms, desc.terms, sizeof(SystemDesc::terms));

		ecs_entity_t system = ecs_system_init(world, &sdesc);
		if (!system) {
			ecs_delete(world, entity);
			context->throw_error_at(at, "failed to register flecs system '%s'", desc.name ? desc.name : "<unnamed>");
		}
	}

	void broadcast(const World& world, ecs_id_t event_id)
	{
		flecs::world fworld(world.world_);
		fworld.event(event_id)
			.id<pulse::EventTag>()
			.entity(fworld.singleton<pulse::SingleHolder>())
			.enqueue();
	}

	void broadcast_with_payload(const World& world, ecs_id_t event_id, const void* data)
	{
		flecs::world fworld(world.world_);
		fworld.event(event_id)
			.id<pulse::EventTag>()
			.entity(fworld.singleton<pulse::SingleHolder>())
			.ctx(data)
			.enqueue();
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

MAKE_TYPE_FACTORY(Term, ecs_term_t);
struct TermAnnotation final : das::ManagedStructureAnnotation<ecs_term_t>
{
	TermAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("Term", ml, "ecs_term_t")
	{
		addField<DAS_BIND_MANAGED_FIELD(id)>("id");
		addField<DAS_BIND_MANAGED_FIELD(inout)>("inout");
	}
};

struct SystemDescAnnotation final : das::ManagedStructureAnnotation<dasPulseECS::SystemDesc>
{
	SystemDescAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("SystemDesc", ml, "dasPulseECS::SystemDesc")
	{
		addField<DAS_BIND_MANAGED_FIELD(name)>("name");
		addField<DAS_BIND_MANAGED_FIELD(dependsOn)>("dependsOn");
		addField<DAS_BIND_MANAGED_FIELD(immediate)>("immediate");
		addField<DAS_BIND_MANAGED_FIELD(terms)>("terms");
	}

	virtual bool isLocal() const override { return true; }
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
		addAnnotation(make_smart<TermAnnotation>(lib));
		addAnnotation(make_smart<SystemDescAnnotation>(lib));
		addAnnotation(make_smart<ModuleContextAnnotation>(lib));

		addExtern<DAS_BIND_FUN(dasPulseECS::create_entity)>(*this, lib, "create_entity", SideEffects::worstDefault, "create_entity")->args({ "world" });
		addExtern<DAS_BIND_FUN(dasPulseECS::destruct_entity)>(*this, lib, "destruct_entity", SideEffects::worstDefault, "destruct_entity")->args({ "world", "entity" });
		addExtern<DAS_BIND_FUN(dasPulseECS::dump_world)>(*this, lib, "dump_world", SideEffects::modifyExternal, "dump_world")->args({ "world" });
		addExtern<DAS_BIND_FUN(dasPulseECS::dump_entity)>(*this, lib, "dump_entity", SideEffects::modifyExternal, "dump_entity")->args({ "entity" });
		addExtern<DAS_BIND_FUN(dasPulseECS::get_single_holder)>(*this, lib, "get_single_holder", SideEffects::modifyExternal, "get_single_holder")->args({ "world" });
		addExtern<DAS_BIND_FUN(dasPulseECS::get_event_tag)>(*this, lib, "get_event_tag", SideEffects::modifyExternal, "get_event_tag")->args({ "world" });
		addExtern<DAS_BIND_FUN(dasPulseECS::build_query)>(*this, lib, "build_query", SideEffects::worstDefault, "build_query")->args({ "world", "query_desc" });
		addExtern<DAS_BIND_FUN(dasPulseECS::query_iter), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "query_iter", SideEffects::worstDefault, "query_iter")->args({ "query" });
		addExtern<DAS_BIND_FUN(dasPulseECS::iter_next)>(*this, lib, "iter_next", SideEffects::worstDefault, "iter_next")->args({ "iter" });
		addExtern<DAS_BIND_FUN(dasPulseECS::iter_field)>(*this, lib, "iter_field", SideEffects::worstDefault, "iter_field")->args({ "iter", "size", "index" });
		addExtern<DAS_BIND_FUN(dasPulseECS::iter_entity)>(*this, lib, "iter_entity", SideEffects::worstDefault, "iter_entity")->args({ "iter", "index" });
		addExtern<DAS_BIND_FUN(dasPulseECS::register_component)>(*this, lib, "register_component", SideEffects::worstDefault, "register_component")->args({ "world", "component_name", "size", "alignment" });
		addExtern<DAS_BIND_FUN(dasPulseECS::set_component)>(*this, lib, "set_component", SideEffects::worstDefault, "set_component")->args({ "world", "entity", "component_id", "size", "data" });
		addExtern<DAS_BIND_FUN(dasPulseECS::get_component)>(*this, lib, "get_component", SideEffects::worstDefault, "get_component")->args({ "world", "entity", "component_id" });
		addExtern<DAS_BIND_FUN(dasPulseECS::get_res)>(*this, lib, "get_res", SideEffects::worstDefault, "get_res")->args({ "world", "component_id" });
		addExtern<DAS_BIND_FUN(dasPulseECS::get_mut_res)>(*this, lib, "get_mut_res", SideEffects::worstDefault, "get_mut_res")->args({ "world", "component_id" });
		addExtern<DAS_BIND_FUN(dasPulseECS::register_system_from_query_expr)>(*this, lib, "register_system_from_query_expr", SideEffects::worstDefault, "register_system_from_query_expr")->args({ "world", "name", "dependson", "query_expr", "fn", "context", "at" });
		addExtern<DAS_BIND_FUN(dasPulseECS::register_system_from_desc)>(*this, lib, "register_system_from_desc", SideEffects::worstDefault, "register_system_from_desc")->args({ "world", "desc", "fn", "context", "at"});
		addExtern<DAS_BIND_FUN(dasPulseECS::broadcast)>(*this, lib, "broadcast", SideEffects::worstDefault, "broadcast")->args({ "world", "event_id" });
		addExtern<DAS_BIND_FUN(dasPulseECS::broadcast_with_payload)>(*this, lib, "broadcast_with_payload", SideEffects::worstDefault, "broadcast_with_payload")->args({ "world", "event_id", "data" });
	}
};

REGISTER_MODULE(ModuleFlecs);

#pragma once

#include "flecs.h"
#include <algorithm>
#include <functional>
#include <initializer_list>
#include <memory>
#include <vector>

// ECS组件
#define PULSE_ECS_COMPONENT
// ECS全局单例组件
#define PULSE_ECS_SINGLETON_COMPONENT
// ECS标签，无成员变量
#define PULSE_ECS_TAG
// ECS事件，Event后缀
#define PULSE_ECS_EVENT
// ECS系统，System后缀
#define PULSE_ECS_SYSTEM(...)
// ECS外部资源
#define PULSE_ECS_RESOURCE
// ECS状态机枚举，INIT=指定初始状态
#define PULSE_ECS_STATE_MACHINE(...)

namespace pulse
{
	struct InitPipeline {};
	struct UpdatePipeline {};
	struct PostUpdatePipeline {};
	struct RenderPipeline {};
	struct ImguiPipeline {};

	template<typename T>
	struct res
	{
	public:
		explicit res(T& value)
			: value(value)
		{
		}

		T& get() const { return value; }

	private:
		T& value;
	};

	PULSE_ECS_TAG
	struct SingleHolder {};

	PULSE_ECS_TAG
		struct EventTag {};

	template<typename T>
	struct singleton_query
	{
	public:
		explicit singleton_query(flecs::world& world)
		{
			singleton_holder = world.singleton<SingleHolder>();
			if (!singleton_holder.has<T>())
				singleton_holder.add<T>();
		}

		const T& get() const
		{
			return singleton_holder.get<T>();
		}

		T& get_mut() const
		{
			return singleton_holder.get_mut<T>();
		}

	private:
		flecs::entity singleton_holder;
	};

	template<typename T>
	struct event_writer
	{
	public:
		explicit event_writer(flecs::world& world)
			: world(world)
		{
		}

		template<typename...C>
		void send(flecs::entity entity)
		{
			world.event<T>()
				.template id<C...>()
				.entity(entity)
				.enqueue();
		}

		template<typename...C>
		void send(flecs::entity entity, const T& payload)
		{
			world.event<T>()
				.template id<C...>()
				.entity(entity)
				.ctx(payload)
				.enqueue();
		}

		void send(flecs::entity entity)
		{
			world.event<T>()
				.template id<EventTag>()
				.entity(entity)
				.enqueue();
		}

		void send(flecs::entity entity, const T& payload)
		{
			world.event<T>()
				.template id<EventTag>()
				.entity(entity)
				.ctx(payload)
				.enqueue();
		}

		void broadcast()
		{
			world.event<T>()
				.template id<EventTag>()
				.entity(world.singleton<pulse::SingleHolder>())
				.enqueue();
		}

		void broadcast(const T& payload)
		{
			world.event<T>()
				.template id<EventTag>()
				.entity(world.singleton<pulse::SingleHolder>())
				.ctx(payload)
				.enqueue();
		}

	private:
		flecs::world& world;
	};

	template<typename T>
	struct event_reader
	{
	public:
		explicit event_reader(const T& event)
			: event(event)
		{
		}

		T read()
		{
			return event;
		}

	private:
		T event;
	};

	struct command_buffer
	{
	public:
		explicit command_buffer(flecs::world& world)
			: world(world)
		{
		}

		void add_singleton(flecs::id_t component) const
		{
			auto single = world.singleton<pulse::SingleHolder>();
			single.add(component);
		}

		template <typename T>
		void set_singleton(const T& value) const
		{
			auto single = world.singleton<pulse::SingleHolder>();
			single.set<T>(value);
		}

		template <typename T>
		void set_singleton(T&& value) const
		{
			auto single = world.singleton<pulse::SingleHolder>();
			single.set<T>(std::forward<T>(value));
		}

		template <typename T>
		void remove_singleton() const
		{
			auto single = world.singleton<pulse::SingleHolder>();
			single.remove<T>();
		}

		void remove_singleton(flecs::entity_t entity) const
		{
			auto single = world.singleton<pulse::SingleHolder>();
			single.remove(entity);
		}

		template <typename... Args>
		flecs::entity entity(Args &&... args) const
		{
			return world.entity<Args...>(std::forward<Args>(args)...);
		}

		void destruct(flecs::entity entity) const
		{
			entity.destruct();
		}

		void defer_suspend()
		{
			world.defer_suspend();
		}

		void defer_resume()
		{
			world.defer_resume();
		}

	private:
		flecs::world& world;
	};

	struct EventRegisterBase
	{
		virtual ~EventRegisterBase() = default;
	};

	template<typename T>
	struct EventRegister : public EventRegisterBase
	{
	public:
		template <typename Func, typename... Payloads>
		void reg(Func&& func, Payloads&&... payloads)
		{
			listeners.emplace_back(
				[
					f = std::forward<Func>(func),
					bound_payloads = std::make_tuple(std::forward<Payloads>(payloads)...)
				](pulse::event_reader<T> eventReader, flecs::world& world) mutable
				{
					std::apply([&](auto&... unpacked_payloads) {
						f(eventReader, world, unpacked_payloads...);
						}, bound_payloads);
				}
			);
		}

		// 返回 observer 实体，便于注册进状态机做 enable/disable
		flecs::observer observe(flecs::world& world)
		{
			return world.observer<EventTag>()
				.template event<T>()
				.ctx(this)
				.each([](flecs::iter& it, size_t i, EventTag)
					{
						auto world = it.world();
						auto c = (EventRegister<T>*)it.ctx();
						const T& event = *(T*)it.param();
						auto eventReader = pulse::event_reader<T>(event);
						c->dispatch(eventReader, world);
					});
		}

	private:
		void dispatch(pulse::event_reader<T> eventReader, flecs::world& world)
		{
			for (auto& listener : listeners)
			{
				listener(eventReader, world);
			}
		}

		std::vector<std::function<void(pulse::event_reader<T>, flecs::world&)>> listeners;
	};

	template<typename T, typename...C>
	struct EntityEventRegister : public EventRegisterBase
	{
	public:
		template <typename Func, typename... Payloads>
		void reg(Func&& func, Payloads&&... payloads)
		{
			listeners.emplace_back(
				[
					f = std::forward<Func>(func),
					bound_payloads = std::make_tuple(std::forward<Payloads>(payloads)...)
				](pulse::event_reader<T> eventReader, flecs::world& world, C&...c) mutable
				{
					std::apply([&](auto&... unpacked_payloads) {
						f(eventReader, world, unpacked_payloads..., c...);
						}, bound_payloads);
				}
			);
		}

		// 返回 observer 实体，便于注册进状态机做 enable/disable
		flecs::observer observe(flecs::world& world)
		{
			return world.observer<C...>()
				.template event<T>()
				.ctx(this)
				.each([](flecs::iter& it, size_t i, C&...c)
					{
						auto world = it.world();
						auto* self = (EntityEventRegister<T, C...>*)it.ctx();
						const T& event = *(T*)it.param();
						auto eventReader = pulse::event_reader<T>(event);
						self->dispatch(eventReader, world, c...);
					});
		}

	private:
		void dispatch(pulse::event_reader<T> eventReader, flecs::world& world, C&...c)
		{
			for (auto& listener : listeners)
			{
				listener(eventReader, world, c...);
			}
		}

		std::vector<std::function<void(pulse::event_reader<T>, flecs::world&, C&...)>> listeners;
	};

	struct EventCenter
	{
	public:
		void register_event(std::unique_ptr<EventRegisterBase> eventRegister)
		{
			eventRegisters.push_back(std::move(eventRegister));
		}

		void clear()
		{
			eventRegisters.clear();
		}

	private:
		std::vector<std::unique_ptr<EventRegisterBase>> eventRegisters;
	};

	// ============================================================
	// 状态机基础设施（配合 PULSE_ECS_STATE_MACHINE 标记的状态枚举）
	//   - 状态以 TState 组件存放在 SingleHolder 单例上
	//   - system/observer 以"启用状态集"注册进来
	//   - 状态变化时 apply() 经 flecs ecs_enable 批量启用/禁用
	//     （flecs 对 observer 有专门的 disable 支持）
	//   - 状态迁移由系统通过 singleton_query<TState> 写出；
	//     一个每帧最先运行的状态管理系统（IMMEDIATE）调用 tick()
	//     检测变化并 apply，使开关在当帧后续系统即生效
	// ============================================================

	// ============================================================
	// 状态机访问器：作为系统签名参数，供系统读取/迁移游戏状态。
	// 状态以 TState 组件存放在 SingleHolder 单例上；迁移由
	// StateMachine 管理系统（每帧最先，IMMEDIATE）下一帧应用开关。
	// [迁移预留：未来生成器新增 KIND.SYSTEM_STATE_MACHINE，
	//  wrapper 生成 auto state = pulse::system_state_machine<TState>(world);]
	// ============================================================

	template<typename TState>
	struct system_state_machine
	{
	public:
		explicit system_state_machine(flecs::world& world)
			: world(world)
		{
		}

		// 当前状态
		TState current() const
		{
			return world.singleton<SingleHolder>().get<TState>();
		}

		// 是否处于给定状态
		bool is(TState state) const
		{
			return current() == state;
		}

		// 显式状态迁移
		void to(TState next)
		{
			world.singleton<SingleHolder>().set<TState>(next);
		}

	private:
		flecs::world& world;
	};

	template<typename TState>
	struct StateMachine
	{
	public:
		void reg(flecs::entity target, std::initializer_list<TState> states)
		{
			entries.push_back({ target, states });
		}

		// run 前调用（此时 world 可写，开关立即生效）：注册组件、写初值、首次 apply
		void init(flecs::world& world, TState initial)
		{
			world.component<TState>();
			system_state_machine<TState> state(world);
			state.to(initial);
			applied = initial;
			apply(world);
		}

		// 状态管理系统每帧调用：检测状态变化并应用开关
		void tick(flecs::world& world)
		{
			system_state_machine<TState> state(world);
			const TState current = state.current();
			if (!(current == applied))
			{
				applied = current;
				apply(world);
			}
		}

	private:
		struct Entry
		{
			flecs::entity target;       // system 或 observer 实体
			std::vector<TState> states; // 启用状态集
		};

		void apply(flecs::world& world)
		{
			for (auto& entry : entries)
			{
				bool enabled = std::find(entry.states.begin(), entry.states.end(), applied) != entry.states.end();
				ecs_enable(world.c_ptr(), entry.target, enabled);
			}
		}

		std::vector<Entry> entries;
		TState applied{};
	};

	template<typename T>
	void registerResource(flecs::world& world, const char* name)
	{
		auto resource = flecs::component<T>(world, name, false);
		resource.add(flecs::Sparse);
		resource.add(flecs::Singleton);
		world.set<T>({});
	}

	template<typename T>
	void registerResource(flecs::world& world, const char* name, T&& value)
	{
		auto resource = flecs::component<T>(world, name, false);
		resource.add(flecs::Sparse);
		resource.add(flecs::Singleton);
		world.set<T>(value);
	}
}

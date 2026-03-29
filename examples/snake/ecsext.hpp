#pragma once

#include "flecs.h"
#include <functional>
#include <memory>

// ECS组件
#define PULSE_ECS_COMPONENT
// ECS全局单例组件
#define PULSE_ECS_SINGLETON_COMPONENT
// ECS标签，无成员变量
#define PULSE_ECS_TAG
// ECS事件，Event后缀
#define PULSE_ECS_EVENT
// ECS系统，System后缀
#define PULSE_ECS_SYSTEM
// ECS外部资源
#define PULSE_ECS_RESOURCE

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
				.id<C...>()
				.entity(entity)
				.enqueue();
		}

		template<typename...C>
		void send(flecs::entity entity, const T& payload)
		{
			world.event<T>()
				.id<C...>()
				.entity(entity)
				.ctx(payload)
				.enqueue();
		}

		void send(flecs::entity entity)
		{
			world.event<T>()
				.id<EventTag>()
				.entity(entity)
				.enqueue();
		}

		void send(flecs::entity entity, const T& payload)
		{
			world.event<T>()
				.id<EventTag>()
				.entity(entity)
				.ctx(payload)
				.enqueue();
		}

		void broadcast()
		{
			world.event<T>()
				.id<EventTag>()
				.entity(world.singleton<pulse::SingleHolder>())
				.enqueue();
		}

		void broadcast(const T& payload)
		{
			world.event<T>()
				.id<EventTag>()
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

		void observe(flecs::world& world)
		{
			world.observer<EventTag>()
				.event<T>()
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

		void observe(flecs::world& world)
		{
			world.observer<C...>()
				.event<T>()
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

	struct ModuleContext
	{
		flecs::world world;
		flecs::entity_t initPipeline;
		flecs::entity_t updatePipeline;
		flecs::entity_t postUpdatePipeline;
		flecs::entity_t renderPipeline;
		flecs::entity_t imguiPipeline;
		pulse::EventCenter* eventManager;
	};

	template<typename T>
	void registerResource(flecs::world& world, const char* name)
	{
		auto keyboardState = flecs::component<T>(world, name, false);
		keyboardState.add(flecs::Sparse);
		keyboardState.add(flecs::Singleton);
		world.set<T>({});
	}

	template<typename T>
	void registerResource(flecs::world& world, const char* name, T&& value)
	{
		auto keyboardState = flecs::component<T>(world, name, false);
		keyboardState.add(flecs::Sparse);
		keyboardState.add(flecs::Singleton);
		world.set<T>(value);
	}
}

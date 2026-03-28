#pragma once

#include "flecs.h"
#include <functional>

namespace pulse
{
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

	struct SingleHolder {};

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

		void send(flecs::entity)
		{

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

		template <typename... Args>
		flecs::entity entity(Args &&... args) const
		{
			return world.entity<Args...>(std::forward<Args>(args)...);
		}

		void destruct(flecs::entity entity) const
		{
			entity.destruct();
		}

	private:
		flecs::world& world;
	};

	template<typename T>
	struct EventRegister
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
				.each([](flecs::iter& it, size_t i, const EventTag& eventTag)
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
}

#pragma once

#include "flecs.h"

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
		explicit singleton_query(flecs::world world)
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
}

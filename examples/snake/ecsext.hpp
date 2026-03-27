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
}

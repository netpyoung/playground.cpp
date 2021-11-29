#pragma once

namespace nf::memory
{
	enum class eAllocationPolicy
	{
		CallConstructorDestructor,
		None,
	};
} // namespace nf::memory
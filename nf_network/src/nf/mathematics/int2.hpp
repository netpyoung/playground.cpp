#pragma once

#include <cstdint>
#pragma warning(push, 0)
#include <functional>
#include <iostream>
#pragma warning(pop)
#pragma warning(disable: 4643)
#pragma warning(disable: 4514)

namespace nf::mathematics
{
	class int2 final
	{
	public:
		int2() = default;
		int2(const int32_t x, const int32_t y);
		int2(const int2& other);
		virtual ~int2() = default;

	public:
		int2& operator=(const int2& other);
		bool operator==(const int2& other) const;
		bool operator!=(const int2& other) const;
		int2 operator+(const int2& other) const;
		int2 operator-(const int2& other) const;
		int2 operator*(const int2& other) const;

	public:
		int32_t X{ 0 };
		int32_t Y{ 0 };

	public:
		friend std::ostream& operator<<(std::ostream& os, const int2& x)
		{
			return os << "(" << x.X << "," << x.Y << ")";
		}
	};
} // namespace common

namespace std
{
	// std::hash - injectable from c++17
	// https://en.cppreference.com/w/cpp/utility/hash

	template <>
	struct hash<nf::mathematics::int2>
	{
		size_t operator()(nf::mathematics::int2 const& k) const
		{
			// http://stackoverflow.com/a/1646913/126995
			size_t res = 17;
			res = res * 31 + hash<int32_t>()(k.X);
			res = res * 31 + hash<int32_t>()(k.Y);
			return res;
		}
	};
} // namespace std
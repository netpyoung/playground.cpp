#include "int2.hpp"

namespace nf::mathematics
{
	int2::int2(const int32_t x, const int32_t y) : X(x), Y(y)
	{
	}

	int2::int2(const int2& o) : X(o.X), Y(o.Y)
	{
	}

	int2& int2::operator=(const int2& o)
	{
		this->X = o.X;
		this->Y = o.Y;
		return *this;
	}

	bool int2::operator==(const int2& o) const
	{
		if (this->X != o.X)
		{
			return false;
		}
		if (this->Y != o.Y)
		{
			return false;
		}
		return true;
	}

	bool int2::operator!=(const int2& o) const
	{
		return !(*this == o);
	}
	int2 int2::operator+(const int2& o) const
	{
		return int2(this->X + o.X, this->Y + o.Y);
	}

	int2 int2::operator-(const int2& o) const
	{
		return int2(this->X - o.X, this->Y - o.Y);
	}

	int2 int2::operator*(const int2& o) const
	{
		return int2(this->X * o.X, this->Y * o.Y);
	}
} // namespace commmon
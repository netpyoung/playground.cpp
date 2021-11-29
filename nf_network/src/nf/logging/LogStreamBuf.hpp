#pragma once
#include <streambuf>

namespace nf::logging
{
	class LogStreamBuf : public std::streambuf
	{
	public:
		inline virtual int_type overflow(int_type ch) override
		{
			return ch;
		}
		size_t pcount() const
		{
			return static_cast<size_t>(pptr() - pbase());
		}
		char* pbase() const
		{
			return std::streambuf::pbase();
		}
	};
} // namespace nf::logging
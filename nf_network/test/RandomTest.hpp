#pragma once

#include <gtest/gtest.h>
#include "nf/system/Random.hpp"
//#include "nf/debug/Assert.hpp"
#include <nf/logging/Logger.hpp>
namespace nf::system::test
{
	TEST(HelloTest, ZeroAndNegativeNos)
	{
		Random random(0);
		std::cout << "Rand: " <<  random.rand(10) << std::endl;
		logging::Logger logger(logging::eLogLevel::Debug);
		/*debug::Assertion assertion(&logger);
		assertion.Assert(true, "%d %d", 99, 88);*/
	}
}
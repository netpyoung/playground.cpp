#include "gtest/gtest.h"
#include "AutoResetEventTest.hpp"
#include "RandomTest.hpp"
#include "EncoderTest.hpp"

int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
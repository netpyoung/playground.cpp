#include "nf/objectpool/ObjectPool.hpp"
#include "nf/objectpool/ObjectPoolLockFree.hpp"
#include "nf/system/collections/concurrent/LockFreeStack.hpp"
#include "nf/objectpool/ObjectPoolTLS.hpp"
#include "nf/memory/eAllocationPolicy.hpp"
#pragma comment(lib,"winmm.lib")
#include <timeapi.h>
#include <thread>
#include <cstdio>
#include <iostream>
#include <cstdint>
#include "nf/system/threading/ManualResetEvent.hpp"
#include "nf/os/windows/input/Keyboard.hpp"
#include "nf/dump/CrashDump.hpp"
#include "nf/assert/Assert.hpp"
#include <vector>
using namespace nf::system::threading;
using namespace nf::system::collections::concurrent;

namespace test
{
	using namespace nf;
	using namespace nf::objectpool;
	using namespace nf::os;


	windows::input::Keyboard gKeyboard;
	dump::CrashDump CrashDump;

	constexpr size_t THREAD_COUNT = 2; // 8
	constexpr size_t POOL_SIZE = 5; // 10000
	/*constexpr size_t THREAD_COUNT = 8;
	constexpr size_t POOL_SIZE = 10000;*/
	constexpr int32_t SLEEP_MS = 0;
	constexpr int64_t DATA = 0x0000000055555555;
	struct AData
	{
	public:
		volatile int64_t Data;

		AData() : Data(0)
		{
			printf("default AData %lld\n", Data);
		}

		AData(int64_t const data) : Data(data)
		{
			printf("constructor AData %lld\n", Data);
		}
		virtual ~AData()
		{
			printf("destructor AData %lld\n", Data);
		}
	};

	struct BData
	{
		volatile int64_t Data{ 0x0000000055555555 };
		volatile int64_t Count{ 0 };
	};

	int main1()
	{
		ObjectPoolTLS<AData, memory::eAllocationPolicy::CallConstructorDestructor> pool(0);
		printf("Test Start\n");

		assert(0 == pool.GetUsedSize());
		assert(0 == pool.GetMaxCapacity());

		AData* const data1 = pool.Rent(1);
		AData* const data2 = pool.Rent(2);
		AData* const data3 = pool.Rent(3);
		assert(3 == pool.GetUsedSize());
		assert(3 == pool.GetMaxCapacity());

		pool.Return(data2);
		pool.Return(data1);
		assert(1 == pool.GetUsedSize());
		assert(3 == pool.GetMaxCapacity());

		AData* const data4 = pool.Rent(4);
		assert(2 == pool.GetUsedSize());
		assert(3 == pool.GetMaxCapacity());

		pool.Return(data3);
		pool.Return(data4);
		assert(0 == pool.GetUsedSize());
		assert(3 == pool.GetMaxCapacity());
		printf("Test Ended\n");
		return 0;
	}

	void TestThread(ManualResetEvent* const quitMre, LockFreeStack<BData*>* const stack, int32_t const repeatMs)
	{
		ObjectPoolTLS<BData, memory::eAllocationPolicy::None> pool(0);
		std::array<BData*, POOL_SIZE> buffer;
		for (int32_t i = 0; i < POOL_SIZE; ++i)
		{
			BData* const data = pool.Rent();
			printf("[alloc] data %p\n", data);
			assert::Assertion::True(data->Count == 0);
			assert::Assertion::True(data->Data == DATA);
			stack->Push(data);
			buffer[i] = data;
		}
		printf("pool GetUsedSize : %I64d\n", pool.GetUsedSize());

		while (true)
		{
			if (quitMre->WaitOne(repeatMs))
			{
				for (int32_t i = 0; i < POOL_SIZE; ++i)
				{
					BData* data = nullptr;
					bool const popResult = stack->TryPop(&data);
					assert(data != nullptr);
					bool const isFree = pool.Return(buffer[i]);
					assert::Assertion::True(isFree);
				}
				break;
			}

			BData* datas[POOL_SIZE] = { nullptr, };
			for (int32_t i = 0; i < POOL_SIZE; ++i)
			{
				BData* data;
				bool const popResult = stack->TryPop(&data);
				assert::Assertion::True(popResult);
				datas[i] = data;
			}

			Sleep(SLEEP_MS);

			for (auto data : datas)
			{
				assert::Assertion::True(data->Count == 0);
				assert::Assertion::True(data->Data == DATA);
			}

			Sleep(SLEEP_MS);

			for (auto data : datas)
			{
				InterlockedIncrement64(&data->Data);
				InterlockedIncrement64(&data->Count);
			}

			Sleep(SLEEP_MS);

			for (auto data : datas)
			{
				assert::Assertion::True(data->Count == 1);
				assert::Assertion::True(data->Data == 0x0000000055555556);
			}

			Sleep(SLEEP_MS);

			for (auto data : datas)
			{
				InterlockedDecrement64(&data->Data);
				InterlockedDecrement64(&data->Count);
			}

			Sleep(SLEEP_MS);

			for (auto data : datas)
			{
				bool const isPush = stack->Push(data);
				assert::Assertion::True(isPush);
			}
		}
	}
	int main2()
	{
		timeBeginPeriod(1);
		std::unique_ptr<ManualResetEvent> const quitMre = std::make_unique<ManualResetEvent>(false);

		LockFreeStack<BData*> pool{ POOL_SIZE * THREAD_COUNT };

		std::array<std::thread, THREAD_COUNT > jobThreads;

		for (size_t i = 0; i < THREAD_COUNT; ++i)
		{
			jobThreads[i] = std::thread(TestThread, quitMre.get(), &pool, 0);
		}

		while (true)
		{
			gKeyboard.UpdateKey(windows::input::eKey::Esc);

			if (gKeyboard.KeyDown(windows::input::eKey::Esc))
			{
				quitMre->Set();
				break;
			}

			Sleep(1000);

			std::cout << pool.GetMaxCapacity() << " : " << pool.GetUsedSize() << std::endl;
		}

		for (std::thread& thread : jobThreads)
		{
			thread.join();
		}
		return 0;
	}

} // namespace test


int main()
{
	return test::main1();
}

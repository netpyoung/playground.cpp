#include "nf/objectpool/ObjectPool.hpp"
#include "nf/objectpool/ObjectPoolLockFree.hpp"
#include "nf/system/collections/concurrent/LockFreeStack.hpp"
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

	int main2()
	{
		ObjectPool<AData, memory::eAllocationPolicy::CallConstructorDestructor> pool(0);
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

	int main4()
	{
		ObjectPoolLockFree<AData, memory::eAllocationPolicy::CallConstructorDestructor> pool(0);
		printf("Test Start\n");

		assert(0 == pool.GetUsedSize());
		assert(0 == pool.GetMaxCapacity());

		AData* const data1 = pool.Rent(1);
		AData* const data2 = pool.Rent(2);
		AData* const data3 = pool.Rent(3);
		// [] | #{1, 2, 3}
		assert(3 == pool.GetUsedSize());
		assert(3 == pool.GetMaxCapacity());

		pool.Return(data2);
		pool.Return(data1);
		// [1, 2] | #{3}
		assert(1 == pool.GetUsedSize());
		assert(3 == pool.GetMaxCapacity());

		AData* const data4 = pool.Rent(4);
		// [2] | #{3, 4}
		assert(2 == pool.GetUsedSize());
		assert(3 == pool.GetMaxCapacity());

		pool.Return(data3);
		pool.Return(data4);
		// [4, 3, 2] | #{}
		assert(0 == pool.GetUsedSize());
		assert(3 == pool.GetMaxCapacity());
		printf("Test Ended\n");
		return 0;
	}

	void AllocFreeThread(ManualResetEvent* const quitMre, ObjectPoolLockFree<AData, memory::eAllocationPolicy::None>* const pool, int32_t const repeatMs)
	{

		int32_t acc = 0;
		std::vector<AData*> datas;
		while (true)
		{
			if (quitMre->WaitOne(repeatMs))
			{
				break;
			}


			for (int32_t i = 0; i < 10; ++i)
			{
				acc++;
				AData* const data = pool->Rent(acc);
				datas.push_back(data);
			}

			for (auto data : datas)
			{
				pool->Return(data);
			}
			datas.clear();
		}
		printf("%d\n", acc);
	}

	void AllocFreeThread2(ManualResetEvent* const quitMre, ObjectPoolLockFree<AData, memory::eAllocationPolicy::None>* const pool, int32_t const repeatMs);

	int main5()
	{
		timeBeginPeriod(1);

		std::unique_ptr<ManualResetEvent> const quitMre = std::make_unique<ManualResetEvent>(false);
		ObjectPoolLockFree<AData, memory::eAllocationPolicy::None> pool(0);
		std::array<std::thread, 8> jobThreads{
			std::thread(AllocFreeThread2, quitMre.get(), &pool, 0),
			std::thread(AllocFreeThread2, quitMre.get(), &pool, 0),
			std::thread(AllocFreeThread2, quitMre.get(), &pool, 0),
			std::thread(AllocFreeThread2, quitMre.get(), &pool, 0),
			std::thread(AllocFreeThread2, quitMre.get(), &pool, 0),
			std::thread(AllocFreeThread2, quitMre.get(), &pool, 0),
			std::thread(AllocFreeThread2, quitMre.get(), &pool, 0),
			std::thread(AllocFreeThread2, quitMre.get(), &pool, 0),
		};
		while (true)
		{
			gKeyboard.UpdateKey(windows::input::eKey::Esc);

			if (gKeyboard.KeyDown(windows::input::eKey::Esc))
			{
				quitMre->Set();
				break;
			}
		}

		for (std::thread& thread : jobThreads)
		{
			thread.join();
		}
		return 0;
	}


	int main6()
	{
		timeBeginPeriod(1);

		LockFreeStack<int32_t> stack{ 100 };
		for (int32_t i = 0; i < 100; ++i)
		{
			stack.Push(i);
		}

		while (!stack.IsEmpty())
		{
			int32_t out = -1;
			stack.TryPop(&out);
			std::cout << out << std::endl;
		}
		return 0;
	}



	void AllocFreeThread2(ManualResetEvent* const quitMre, ObjectPoolLockFree<AData, memory::eAllocationPolicy::None>* const pool, int32_t const repeatMs)
	{
		constexpr size_t MAX = 10000;

		AData* datas[MAX];

		while (true)
		{
			if (quitMre->WaitOne(repeatMs))
			{
				break;
			}

			for (int32_t i = 0; i < MAX; ++i)
			{
				AData* const data = pool->Rent(i);
				datas[i] = data;
			}

			for (auto data : datas)
			{
				if (!pool->Return(data))
				{
					CrashDump.Crash();
				}
			}
		}
	}


	void AllocFreeThread3(ManualResetEvent* const quitMre, ObjectPoolLockFree<BData, memory::eAllocationPolicy::None>* const pool, int32_t const repeatMs)
	{
		constexpr int32_t SLEEP_MS = 1;
		constexpr size_t MAX = 10000;
		constexpr int64_t DATA = 0x0000000055555555;

		BData* datas[MAX];

		while (true)
		{
			if (quitMre->WaitOne(repeatMs))
			{
				break;
			}

			for (int32_t i = 0; i < MAX; ++i)
			{
				BData* const data = pool->Rent();

				assert::Assertion::True(data->Count == 0);
				assert::Assertion::True(data->Data == DATA);

				datas[i] = data;
			}

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
				assert::Assertion::True(data->Count == 0);
				assert::Assertion::True(data->Data == DATA);
			}

			for (auto data : datas)
			{
				if (!pool->Return(data))
				{
					CrashDump.Crash();
				}
			}
		}
	}
	int main7()
	{
		timeBeginPeriod(1);
		std::unique_ptr<ManualResetEvent> const quitMre = std::make_unique<ManualResetEvent>(false);
		ObjectPoolLockFree<BData, memory::eAllocationPolicy::None> pool(80000);
		std::array<std::thread, 8> jobThreads{
			std::thread(AllocFreeThread3, quitMre.get(), &pool, 0),
			std::thread(AllocFreeThread3, quitMre.get(), &pool, 0),
			std::thread(AllocFreeThread3, quitMre.get(), &pool, 0),
			std::thread(AllocFreeThread3, quitMre.get(), &pool, 0),
			std::thread(AllocFreeThread3, quitMre.get(), &pool, 0),
			std::thread(AllocFreeThread3, quitMre.get(), &pool, 0),
			std::thread(AllocFreeThread3, quitMre.get(), &pool, 0),
			std::thread(AllocFreeThread3, quitMre.get(), &pool, 0),
		};
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


//int main()
//{
//	return test::main5();
//}

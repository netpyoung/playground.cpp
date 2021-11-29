#include "nf/objectpool/ObjectPool.hpp"
#include "nf/objectpool/ObjectPoolLockFree.hpp"
#include "nf/system/collections/concurrent/LockFreeStack.hpp"
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

namespace test2
{
	// 테스트 목적
	// - 넣은 데이터 개수와 뽑은 데이터 개수의 일치 확인
	// - 데이터를 넣었다가 뽑은 뒤에 이 데이터를 다른이가 메모리를 사용하는지 확인 (2중으로 뽑히는지 확인)
	// 테스트 목적
	//
	// - 할당된 메모리를 또 할당 하는가 ?
	// - 잘못된 메모리를 할당 하는가 ?

	// 모든 데이터는 0x0000000055555555 으로 미리 초기화 함.
	//struct Sample
	//{
	//	volatile int64_t Data{ 0x0000000055555555 };
	//	volatile int64_t Count{ 0 };
	//};

	// 여러개의 스레드에서 일정수량의 Rent 과 Return 를 반복적으로 함
	// 0. Rent (스레드당 10000 개 x 10 개 스레드 총 10만개)
	// 1. 0x0000000055555555 과 0 이 맞는지 확인.
	// 2. Interlocked + 1 (Data + 1 / Count + 1)
	// 3. 약간대기
	// 4. 여전히 0x0000000055555556 과 1 이 맞는지 확인.
	// 5. Interlocked - 1 (Data - 1 / Count - 1)
	// 6. 약간대기
	// 7. 0x0000000055555555 과 Count == 0 확인.
	// 8. Return
	// 반복.

	using namespace nf;
	using namespace nf::objectpool;
	using namespace nf::os;

	windows::input::Keyboard gKeyboard;
	dump::CrashDump CrashDump;

	//constexpr size_t THREAD_COUNT = 2; // 8
	//constexpr size_t POOL_SIZE = 5; // 10000
	constexpr size_t THREAD_COUNT = 8;
	constexpr size_t POOL_SIZE = 10000;
	constexpr int32_t SLEEP_MS = 0;
	constexpr int64_t DATA = 0x0000000055555555;

	struct BData
	{
		volatile int64_t Data{ DATA };
		volatile int64_t Count{ 0 };
	};

	void TestThread(ManualResetEvent* const quitMre, LockFreeStack<BData*>* const stack, int32_t const repeatMs)
	{
		ObjectPoolLockFree<BData, memory::eAllocationPolicy::None> pool(0);

		for (int32_t i = 0; i < POOL_SIZE; ++i)
		{
			BData* const data = pool.Rent();
			assert::Assertion::True(data->Count == 0);
			assert::Assertion::True(data->Data == DATA);
			stack->Push(data);
		}

		while (true)
		{
			if (quitMre->WaitOne(repeatMs))
			{
				for (int32_t i = 0; i < POOL_SIZE; ++i)
				{
					BData* data;
					bool const popResult = stack->TryPop(&data);
					bool const isFree = pool.Return(data);
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
	int main()
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

} // namespace test2
//
//int main()
//{
//	return test2::main();
//}
//int main()
//{
//	return test2::main();
//	//int64_t x = -1;
//	//uint64_t a = static_cast<uint64_t>(x);
//	//int64_t b = static_cast<uint64_t>(a);
//	//std::cout << x << std::endl;
//	//std::cout << a << std::endl;
//	//std::cout << b << std::endl;
//
//	//std::cout << (a > 0) << std::endl;
//	//std::cout << (b > 0) << std::endl;
//}
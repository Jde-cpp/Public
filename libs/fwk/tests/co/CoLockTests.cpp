#include <jde/fwk/co/CoLock.h>
#include <jde/fwk/process/execution.h>
#include <atomic>

#define let const auto

namespace Jde::Tests{
	struct CoLockTests : public ::testing::Test{
	protected:
		Ω SetUpTestCase()ι->void{
			Execution::Run();
		}
	};

	Ω single( CoLock& lock, std::atomic<uint>& completed )->LockAwait::Task{
		auto guard = co_await lock.Lock();
		guard.unlock();//explicit unlock - the dtor must not release a second time.
		++completed;
	}

	Ω hold( CoLock& lock, std::atomic<uint>& holders, std::atomic<bool>& overlapped, std::atomic<uint>& completed )->LockAwait::Task{
		{
			auto guard = co_await lock.Lock();
			if( ++holders!=1 )
				overlapped = true;
			std::this_thread::sleep_for( 100us );//hold long enough for other threads to queue behind us.
			--holders;
		}//release before ++completed so the test can't tear down `lock` mid-Clear.
		++completed;
	}

	TEST_F( CoLockTests, SequentialReacquire ){
		CoLock lock;
		std::atomic<uint> completed{};
		for( uint i=0; i<3; ++i ){
			single( lock, completed );
			let deadline = steady_clock::now()+5s;
			while( completed!=i+1 && steady_clock::now()<deadline )
				std::this_thread::sleep_for( 1ms );
			ASSERT_EQ( completed, i+1 ) << "lock could not be reacquired after release " << i;
		}
	}

	// Regression: CoLock::Clear resumed the next waiter inline while holding _mutex - when that
	// waiter released its guard, Clear re-entered and relocked the non-recursive mutex on the same
	// thread. Any queue depth >=2 deadlocked. Contending from many threads forces queueing; every
	// waiter must complete and no two may hold the lock at once.
	TEST_F( CoLockTests, MutualExclusion ){
		CoLock lock;
		std::atomic<uint> holders{}; std::atomic<bool> overlapped{}; std::atomic<uint> completed{};
		constexpr uint total{ 64 };
		{
			vector<std::jthread> threads;
			for( uint i=0; i<total; ++i )
				threads.emplace_back( [&](){ hold(lock, holders, overlapped, completed); } );
		}//launchers join here; suspended coroutines resume on the executor.
		let deadline = steady_clock::now()+15s;
		while( completed<total && steady_clock::now()<deadline )
			std::this_thread::sleep_for( 1ms );
		ASSERT_EQ( completed.load(), total ) << "waiter chain stalled - release deadlock or lost wakeup";
		ASSERT_FALSE( overlapped.load() ) << "two coroutines held the lock simultaneously";
	}
}

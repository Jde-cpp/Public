#include <jde/fwk/co/CoLock.h>
#include <jde/fwk/co/LockKey.h>
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

	//deliberately not named waitFor: ExecutionTests.cpp already has a Jde::Tests::waitFor with a different deadline,
	//and two differing definitions of one template in one namespace is an ODR violation nothing would report.
	Ŧ untilTimeout( T predicate )ι->bool{
		let deadline = steady_clock::now()+15s;
		while( !predicate() && steady_clock::now()<deadline )
			std::this_thread::sleep_for( 1ms );
		return predicate();
	}

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

	//TryLockKey: the non-blocking half of LockKeyAwait, for callers that cannot wait on a holder that may never resume -
	//e.g. ProtoLog::Shutdown, which runs after the executor has been destroyed.
	TEST_F( CoLockTests, TryLockKeyIsExclusive ){
		let key = string{ "CoLockTests.TryLockKeyIsExclusive" };
		auto first = TryLockKey( key );
		ASSERT_TRUE( first );
		EXPECT_FALSE( TryLockKey(key) ) << "the key was handed out twice";
		first.reset();
		EXPECT_TRUE( TryLockKey(key) ) << "released, so it must be acquirable again";
	}

	//The reason LockKeyAwait itself cannot be used to probe: its await_ready enqueues *before* it answers, so abandoning
	//it on a false answer would leave a placeholder nothing pops and the key would be locked for the rest of the process.
	//A failed TryLockKey must leave the queue exactly as it found it.
	TEST_F( CoLockTests, FailedTryLockKeyDoesNotPoisonTheKey ){
		let key = string{ "CoLockTests.FailedTryLockKeyDoesNotPoisonTheKey" };
		auto held = TryLockKey( key );
		ASSERT_TRUE( held );
		for( uint i=0; i<8; ++i )
			EXPECT_FALSE( TryLockKey(key) );
		held.reset();
		auto after = TryLockKey( key );
		EXPECT_TRUE( after ) << "the failed attempts left an entry queued behind the holder";
	}

	Ω holdKey( string key, std::atomic<uint>& holders, std::atomic<bool>& overlapped, std::atomic<uint>& completed )->LockKeyAwait::Task{
		{
			auto guard = co_await LockKeyAwait{ key };
			if( ++holders!=1 )
				overlapped = true;
			std::this_thread::sleep_for( 100us );//hold long enough for other threads to queue behind us.
			--holders;
		}//release before ++completed, so the count cannot run ahead of the queue state the test inspects afterwards.
		++completed;
	}

	//MutualExclusion above covers CoLock; LockKeyAwait is the other, string-keyed lock - the one ProtoLog, the archive
	//and the file writes use - and it had no assertion of its own.  FileTests drives it hard but detects a lost wakeup
	//only as a hung suite; this names the failure instead.  Three distinct ways to break it all land here: two holders
	//at once, a Release that does not Post the next waiter, and a waiter whose placeholder never becomes a handle
	//(the await_ready-then-Suspend window, which 64 threads contending on one key is what actually exercises).
	TEST_F( CoLockTests, LockKeyMutualExclusion ){
		let key = string{ "CoLockTests.LockKeyMutualExclusion" };
		std::atomic<uint> holders{}; std::atomic<bool> overlapped{}; std::atomic<uint> completed{};
		constexpr uint total{ 64 };
		{
			vector<std::jthread> threads;
			for( uint i=0; i<total; ++i )
				threads.emplace_back( [&](){ holdKey(key, holders, overlapped, completed); } );
		}//launchers join here; suspended coroutines resume on the executor.
		ASSERT_TRUE( untilTimeout([&]{ return completed.load()==total; }) ) << "waiter chain stalled at " << completed.load() << " of " << total << " - lost wakeup in Release, or a placeholder nothing pops";
		ASSERT_FALSE( overlapped.load() ) << "two coroutines held the key simultaneously";
		//every holder released, so the key must be acquirable.  This is the observable end of the empty-deque erase at
		//the bottom of Release - not that the map entry is gone (_coLocks is a file-static, and TryLockKey reads an
		//empty deque and an absent one identically) but that no waiter left an entry behind it.
		EXPECT_TRUE( TryLockKey(key).has_value() ) << "the key is still held after all 64 holders released";
	}

	//separate parameters so -Wself-move cannot see the two expressions as identical.
	Ω assignFrom( CoLockGuard& lhs, CoLockGuard& rhs )ι->void{ lhs = move(rhs); }

	//CoLockGuard::operator= releases what it is overwriting; without that the overwritten key stays locked for the rest
	//of the process, which is the bug the operator was written to fix.  Nothing in the repo move-assigns one - FileTests
	//only moves a guard into a coroutine parameter, which is the move *constructor*.
	//The keys are short on purpose: the x.Key.clear() after `Key = move(x.Key)` can only matter inside the small-string
	//buffer, where some implementations leave a moved-from string holding its characters - which is what the move
	//*constructor* above it documents, and it would make the moved-from guard release a key it had just handed away.
	//Measured, not assumed: dropping that clear() and running this leaves the assertion green, because the MSVC STL
	//empties a moved-from string either way.  The assertion is kept for the libc++ build, where it can fire.
	TEST_F( CoLockTests, CoLockGuardMoveAssignReleasesTheOverwrittenKey ){
		let keyA = string{ "lk.mvA" }, keyB = string{ "lk.mvB" };
		auto a = TryLockKey( keyA );
		auto b = TryLockKey( keyB );
		ASSERT_TRUE( a && b );

		*a = move( *b );
		EXPECT_TRUE( TryLockKey(keyA).has_value() ) << "the overwritten key was left locked forever";
		EXPECT_FALSE( TryLockKey(keyB).has_value() ) << "the assigned-to guard did not take the key over";
		b.reset();//the moved-from guard is disengaged, so its dtor must not release a key it handed away.
		EXPECT_FALSE( TryLockKey(keyB).has_value() ) << "the moved-from guard released a key it no longer owns";
		a.reset();
		EXPECT_TRUE( TryLockKey(keyB).has_value() ) << "the surviving guard did not release on destruction";

		//self-assignment has to be a no-op: without the this!=&x guard, Release() runs first and the guard then adopts
		//its own just-cleared members, so the key is released while a live guard still believes it holds it.
		auto self = TryLockKey( string{"lk.self"} );
		let keySelf = string{ "lk.self" };
		ASSERT_TRUE( self );
		assignFrom( *self, *self );
		EXPECT_FALSE( TryLockKey(keySelf).has_value() ) << "self-assignment released the key";
		self.reset();
		EXPECT_TRUE( TryLockKey(keySelf).has_value() );
	}

	//CoGuard's ctor is private, so the only way to hold one outside a coroutine is to have a coroutine hand it over.
	Ω acquire( CoLock& lock, sp<optional<CoGuard>> out, sp<std::atomic<bool>> done )->LockAwait::Task{
		auto guard = co_await lock.Lock();
		out->emplace( move(guard) );
		*done = true;
	}
	Ω holdOnce( CoLock& lock, sp<std::atomic<bool>> acquired )->LockAwait::Task{
		auto guard = co_await lock.Lock();
		*acquired = true;
	}

	//the CoLock twin of the test above, and the same fixed bug: overwriting a CoGuard without clearing its lock first
	//would leave that lock held forever.  A CoLock has no non-blocking probe, so "released" is observed by a waiter
	//that gets in and "held" by one that does not.
	TEST_F( CoLockTests, CoGuardMoveAssignReleasesTheOverwrittenLock ){
		CoLock a, b;
		auto guardA = ms<optional<CoGuard>>(), guardB = ms<optional<CoGuard>>();
		auto doneA = ms<std::atomic<bool>>(), doneB = ms<std::atomic<bool>>();
		acquire( a, guardA, doneA );//both locks are free, so neither await suspends: these run inline on this thread.
		acquire( b, guardB, doneB );
		ASSERT_TRUE( untilTimeout([&]{ return doneA->load() && doneB->load(); }) );
		ASSERT_TRUE( *guardA && *guardB );

		**guardA = move( **guardB );
		auto gotA = ms<std::atomic<bool>>(), gotB = ms<std::atomic<bool>>();
		holdOnce( a, gotA );
		holdOnce( b, gotB );
		EXPECT_TRUE( untilTimeout([&]{ return gotA->load(); }) ) << "the overwritten lock was left held forever";
		guardB->reset();//disengaged by the assignment - releasing here would hand b away behind the guard that owns it.
		std::this_thread::sleep_for( 250ms );//a wrongly-released lock resumes its waiter through the executor, so the negative needs settling time or it just reads the race.
		EXPECT_FALSE( gotB->load() ) << "b was released by the moved-from guard, or never taken over";

		guardA->reset();
		EXPECT_TRUE( untilTimeout([&]{ return gotB->load(); }) ) << "the surviving guard did not release b on destruction";
	}
}

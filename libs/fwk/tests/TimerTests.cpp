#include <jde/fwk/co/Await.h>
#include <jde/fwk/co/Timer.h>
#include <jde/fwk/utils/Stopwatch.h>
#include <jde/fwk/utils/mathUtils.h>

#define let const auto
namespace Jde::Tests{
	using std::chrono::microseconds;
	constexpr ELogTags _tags{ ELogTags::Test };
	struct TimerTests : public ::testing::Test
	{};

	uint _successCount{};
	uint _canceledCount{};
	Ω test( atomic_flag& done, uint i )ι->TimerAwait::Task{
		let delay = Math::Random()%200;
		let kill = Math::Random()%100;
		atomic_flag threadDone;
		atomic_flag threadStart;
		auto timer = mu<DurationTimer>( microseconds(delay) );
		std::jthread t{ [&timer, kill, &threadDone, &threadStart]()ι->void {
			threadStart.test_and_set();
			threadStart.notify_all();
			std::this_thread::sleep_for( microseconds(kill) );
			timer->Cancel();
			threadDone.test_and_set();
			threadDone.notify_all();
		} };
		threadStart.wait( false );
		steady_clock::time_point start = steady_clock::now();
		auto result = co_await *timer;
		steady_clock::time_point end = steady_clock::now();
		TRACE( "[{}]completed: {}, delay: {}, kill: {}, time: {}", hex(i), result.has_value() ? "true" : "canceled", delay, kill, duration_cast<microseconds>(end - start).count() );
		if( result.has_value() )
			++_successCount;
		else
			++_canceledCount;

		threadDone.wait( false );
		done.test_and_set();
		done.notify_all();
	}
	TEST_F( TimerTests, General ){
		constexpr uint testCount = 4096*2;
		for( uint i=0; i<testCount; ++i ){
			atomic_flag done;
			test( done, i );
			done.wait( false );
		}
		TRACE( "success: {}, canceled: {}", _successCount, _canceledCount );
		ASSERT_TRUE( _successCount + _canceledCount == testCount );
	}
}
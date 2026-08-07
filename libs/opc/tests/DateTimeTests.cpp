//UADateTime converts between UA_DateTime (100ns ticks since 1601), the {seconds,nanos} json the gateway publishes, and
//protobuf Timestamp/Duration.  DateTime.h itself arrives through the precompiled header - it has no include guard.
#include <limits>
#include <gtest/gtest.h>

#define let const auto

namespace Jde::Opc::Tests{
	constexpr UA_Int64 unixSeconds{ 1700000000 }; //2023-11-14T22:13:20Z - no leap second, no DST edge, comfortably in range.

	TEST( DateTimeTests, UaRoundTrip ){
		let ua = UA_DateTime_fromUnixTime( unixSeconds );
		EXPECT_EQ( UADateTime{ua}.UA(), ua );
	}

	TEST( DateTimeTests, ToJsonSplitsIntoSecondsAndNanos ){
		let j = UADateTime{ UA_DateTime_fromUnixTime(unixSeconds) }.ToJson();
		EXPECT_EQ( j.at("seconds").to_number<int64_t>(), unixSeconds );
		EXPECT_EQ( j.at("nanos").to_number<int64_t>(), 0 );
	}

	//ToJson emits plain numbers; the ctor also accepts the protobufjs Long form, an ISO string and bare milliseconds.
	TEST( DateTimeTests, JsonRoundTrip ){
		let ua = UA_DateTime_fromUnixTime( unixSeconds );
		let j = UADateTime{ua}.ToJson();
		EXPECT_EQ( UADateTime{jvalue{j}}.UA(), ua );
	}

	TEST( DateTimeTests, FromJsonForms ){
		let expected = UA_DateTime_fromUnixTime( unixSeconds );
		EXPECT_EQ( UADateTime{jvalue{"2023-11-14T22:13:20Z"}}.UA(), expected );
		EXPECT_EQ( UADateTime{jvalue{unixSeconds*1000}}.UA(), expected ); //a bare number is milliseconds.
		EXPECT_THROW( UADateTime{jvalue{true}}, Exception );
	}

	//A whole number of microseconds on purpose: the windows/clang ctor stores microseconds, the gcc one nanoseconds, so
	//a sub-microsecond tick would round differently per platform.
	TEST( DateTimeTests, SubSecondPrecisionSurvivesToJson ){
		let ua = UA_DateTime_fromUnixTime( unixSeconds ) + 12345670; //+1.234567s in 100ns ticks.
		let j = UADateTime{ua}.ToJson();
		EXPECT_EQ( j.at("seconds").to_number<int64_t>(), unixSeconds+1 );
		EXPECT_EQ( j.at("nanos").to_number<int64_t>(), 234567000 );
	}

	TEST( DateTimeTests, ProtobufRoundTrip ){
		let ua = UA_DateTime_fromUnixTime( unixSeconds );
		let timestamp = UADateTime{ua}.ToProto();
		EXPECT_EQ( timestamp.seconds(), unixSeconds );
		EXPECT_EQ( timestamp.nanos(), 0 );
		EXPECT_EQ( UADateTime{timestamp}.UA(), ua );

		let duration = UADateTime{ua}.ToDuration();
		EXPECT_EQ( duration.seconds(), unixSeconds );
		EXPECT_EQ( UADateTime{duration}.UA(), ua );
	}

	// ---- the review's open findings.  See main.cpp for why these are disabled. -------------------------------------

	//#12 predicts std::terminate here: ToParts() is Ι but calls Chrono::ToTimePoint, which is ε, and passes (int16)dts.year
	//into a uint16_t parameter, so a pre-year-0 timestamp wraps to 33768+ and THROW_IFSL(!ymd.ok()) fires inside a noexcept
	//frame.  That did not reproduce on win-clang: the ctor there stores microseconds - _time{Epoch()+microseconds{(dt-_ua1970)/10}} -
	//and the subtraction overflows first, so ToParts gets a scrambled but in-range date and returns quietly.  What is
	//demonstrably broken is the conversion itself - INT64_MIN and INT64_MAX both come back as seconds=6802270473.  The
	//linux ctor takes the *100 nanoseconds branch and overflows differently, so the terminate may well be live there.
	//Either way the fix is the one the review asks for: drop the calendar round trip, split _time - Epoch() as a duration,
	//and range-check the input.
	TEST( DateTimeTests, DISABLED_R2_12_ExtremeTimestampsDoNotCollapse ){
		let low = UADateTime{ std::numeric_limits<UA_Int64>::min() }.ToJson();
		let high = UADateTime{ std::numeric_limits<UA_Int64>::max() }.ToJson();
		EXPECT_NE( serialize(low), serialize(high) );
	}
}

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

	//A whole number of microseconds on purpose.  The floor is TimePoint's own Duration, which is 100ns under the msvc stl
	//and microseconds under libc++ - so a sub-microsecond tick still rounds differently per platform, just for a reason
	//that is now about the clock rather than about which compiler ran.
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

	//#12 (fixed).  The finding predicted std::terminate - ToParts() was Ι but called Chrono::ToTimePoint, which is ε, and
	//passed (int16)dts.year into a uint16_t parameter, so a pre-year-0 timestamp wrapped to 33768+ and THROW_IFSL(!ymd.ok())
	//fired inside a noexcept frame.  That never reproduced here: `dt - _ua1970` overflowed first, so ToParts got a
	//scrambled but in-range date and returned quietly - INT64_MIN and INT64_MAX both came back as seconds=6802270473.
	//Both halves are gone: ToParts is a duration split, and the input is clamped to the span TimePoint can hold.
	TEST( DateTimeTests, ExtremeTimestampsDoNotCollapse ){
		let low = UADateTime{ std::numeric_limits<UA_Int64>::min() }.ToJson();
		let high = UADateTime{ std::numeric_limits<UA_Int64>::max() }.ToJson();
		EXPECT_NE( serialize(low), serialize(high) );
		EXPECT_LT( low.at("seconds").to_number<int64_t>(), 0 );  //clamped, but still on the right side of 1970...
		EXPECT_GT( high.at("seconds").to_number<int64_t>(), 0 ); //...and monotone, rather than the same wrapped date.
		EXPECT_GE( low.at("nanos").to_number<int64_t>(), 0 );    //a protobuf Timestamp needs nanos in [0,1e9)...
		EXPECT_LT( high.at("nanos").to_number<int64_t>(), 1'000'000'000 );
	}

	//L10: the json ctor summed `seconds{} + nanoseconds{}`, whose common type is int64 *nanoseconds*, so the seconds
	//scaling overflowed past ~292 years - undefined, and reachable from any client write to a DATETIME node.  Each term
	//now converts into TimePoint's own unit separately, which reaches the full span the clock can represent.
	TEST( DateTimeTests, JsonSecondsPastTheNanosecondRange ){
		let at = []( int64_t s, int64_t ns ){ return UADateTime{ jvalue{jobject{ {"seconds",s}, {"nanos",ns} }} }; };
		constexpr int64_t past292Years{ 100'000'000'000 };//~3170 years, an order past where the old sum wrapped.
		EXPECT_EQ( at(past292Years,0).ToJson().at("seconds").to_number<int64_t>(), past292Years );
		EXPECT_EQ( at(-past292Years,0).ToJson().at("seconds").to_number<int64_t>(), -past292Years );

		//Beyond what a TimePoint can hold it throws rather than wrapping to an arbitrary date.
		EXPECT_THROW( at(std::numeric_limits<int64_t>::max(), 0), Exception );
		EXPECT_THROW( at(std::numeric_limits<int64_t>::min(), 0), Exception );
		EXPECT_THROW( UADateTime{jvalue{std::numeric_limits<int64_t>::max()}}, Exception );//the bare-millisecond form too.

		//What ToJson emits reads back, up to and including #12's clamped *upper* extreme.
		let high = UADateTime{ std::numeric_limits<UA_Int64>::max() }.ToJson();
		EXPECT_NO_THROW( UADateTime{jvalue{high}} ) << serialize( high );

		//The lower one does not, and that is deliberate rather than an oversight.  #12 clamps INT64_MIN to the exact floor
		//of what a TimePoint holds, and ToParts floors its split so nanos stay non-negative - which leaves a seconds term
		//one second below what scales back into the clock's unit, with positive nanos meant to pull it up.  Reassembling
		//seconds-first cannot see that, so the value is rejected instead of wrapping to an arbitrary date as it used to.
		//The input is UA_DateTime's "no value" sentinel, not a time anyone recorded.
		let low = UADateTime{ std::numeric_limits<UA_Int64>::min() }.ToJson();
		EXPECT_THROW( UADateTime{jvalue{low}}, Exception ) << serialize( low );
	}

	//Before 1970 the seconds go negative but the nanos must not - floor, not truncation toward zero.
	TEST( DateTimeTests, PreEpochSplitsWithNonNegativeNanos ){
		let ua = UA_DateTime_fromUnixTime( -1 )+2'500'000;//1969-12-31T23:59:59.25Z
		let j = UADateTime{ua}.ToJson();
		EXPECT_EQ( j.at("seconds").to_number<int64_t>(), -1 );
		EXPECT_EQ( j.at("nanos").to_number<int64_t>(), 250'000'000 );
		EXPECT_EQ( UADateTime{jvalue{j}}.UA(), ua );
	}
}

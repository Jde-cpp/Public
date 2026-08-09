#include <jde/fwk/chrono.h>

#define let const auto

namespace Jde::Tests{
	using namespace std::chrono;

	TEST( ChronoTests, DurationRoundTripMonths ){
		let d = duration_cast<Duration>( months{5} );
		EXPECT_EQ( Chrono::ToString(d), "P5M" );
		EXPECT_EQ( Chrono::ToDuration(Chrono::ToString(d)), d );
	}

	TEST( ChronoTests, DurationDays ){
		EXPECT_EQ( Chrono::ToDuration(sv{"P4D"}), duration_cast<Duration>(days{4}) );
	}

	TEST( ChronoTests, DurationRoundTripMixed ){
		let d = duration_cast<Duration>(years{2}) + duration_cast<Duration>(months{3}) + duration_cast<Duration>(days{4}) + hours{5} + minutes{6} + seconds{7};
		let s = Chrono::ToString(d);
		EXPECT_EQ( Chrono::ToDuration(s), d ) << "str=" << s;
	}

	// Regression: the sub-second remainder was emitted with no unit suffix ("PT1S500"), which ToDuration dropped.
	TEST( ChronoTests, DurationRoundTripSubSecond ){
		let d = duration_cast<Duration>( milliseconds{1500} );
		EXPECT_EQ( Chrono::ToString(d), "PT1.5S" );
		EXPECT_EQ( Chrono::ToDuration(Chrono::ToString(d)), d );
		EXPECT_EQ( Chrono::ToString(duration_cast<Duration>(milliseconds{-1050})), "PT-1.05S" );
		EXPECT_EQ( Chrono::ToDuration(sv{"PT-1.05S"}), duration_cast<Duration>(milliseconds{-1050}) );
		EXPECT_EQ( Chrono::ToDuration(Chrono::ToString(duration_cast<Duration>(milliseconds{50}))), duration_cast<Duration>(milliseconds{50}) );
	}

	// Precision must trim the rendered string, not zero-fill it: %T's digit count comes from the time_point's period,
	// so rounding the value back into TimePoint left "…:05.123000Z" for millisecond precision.
	TEST( ChronoTests, IsoStringPrecisionTrimsOutput ){
		let tp = Chrono::ToTimePoint( 2024, 1, 2, 3, 4, 5, duration_cast<Duration>(microseconds{123456}) );
		EXPECT_EQ( ToIsoString<milliseconds>(tp), "2024-01-02T03:04:05.123Z" );
		EXPECT_EQ( ToIsoString<seconds>(tp), "2024-01-02T03:04:05Z" );
		EXPECT_EQ( ToIsoString<minutes>(tp), "2024-01-02T03:04Z" );
		EXPECT_EQ( ToIsoString<hours>(tp), "2024-01-02T03Z" );
		EXPECT_EQ( ToIsoString<days>(tp), "2024-01-02" ) << "a bare date carries no zone designator";
		EXPECT_EQ( ToIsoString<microseconds>(tp), "2024-01-02T03:04:05.123456Z" );
		//Precision defaults to void: the time_point's own precision, untouched.
		EXPECT_EQ( ToIsoString(tp), "2024-01-02T03:04:05.123456Z" );
		EXPECT_EQ( ToIsoString<>(tp), ToIsoString(tp) );
	}

	// steady_clock has no calendar, so it is converted to Clock first - it used to need an explicit specialization and
	// now falls out of the one template, at every precision rather than just the default.
	TEST( ChronoTests, IsoStringSteadyClock ){
		let steady = steady_clock::now();
		EXPECT_EQ( ToIsoString(steady).size(), ToIsoString(Clock::now()).size() );
		EXPECT_EQ( ToIsoString<days>(steady).size(), 10u ) << "yyyy-mm-dd";
		EXPECT_EQ( ToIsoString<seconds>(steady).size(), 20u ) << "yyyy-mm-ddThh:mm:ssZ";
	}

	// floor, not round: a cert expiring at 18:00 expires *that* day, and a rendered expiration must never read later
	// than the instant it stands for.
	TEST( ChronoTests, IsoStringPrecisionTruncates ){
		let evening = Chrono::ToTimePoint( 2024, 1, 2, 18, 44, 59, duration_cast<Duration>(milliseconds{900}) );
		EXPECT_EQ( ToIsoString<days>(evening), "2024-01-02" );
		EXPECT_EQ( ToIsoString<hours>(evening), "2024-01-02T18Z" );
		EXPECT_EQ( ToIsoString<minutes>(evening), "2024-01-02T18:44Z" );
		EXPECT_EQ( ToIsoString<seconds>(evening), "2024-01-02T18:44:59Z" );
	}

	TEST( ChronoTests, IsoStringRoundTrips ){
		let tp = Chrono::ToTimePoint( 2024, 1, 2, 3, 4, 5 );
		EXPECT_EQ( Chrono::ToTimePoint(ToIsoString<seconds>(tp)), tp );
		EXPECT_EQ( Chrono::ToTimePoint(ToIsoString<milliseconds>(tp)), tp );
	}

	TEST( ChronoTests, ToTimePointZulu ){
		EXPECT_EQ( Chrono::ToTimePoint("2024-01-02T03:04:05Z"), Chrono::ToTimePoint("2024-01-02T03:04:05") );
	}

	TEST( ChronoTests, ToTimePointOffset ){
		//+05:00 => local is 5h ahead of UTC, so UTC = 03:04:05 - 5h = 2024-01-01T22:04:05.
		EXPECT_EQ( Chrono::ToTimePoint("2024-01-02T03:04:05+05:00"), Chrono::ToTimePoint("2024-01-01T22:04:05") );
		//-03:00 => local is 3h behind UTC, so UTC = 03:04:05 + 3h = 06:04:05.
		EXPECT_EQ( Chrono::ToTimePoint("2024-01-02T03:04:05-03:00"), Chrono::ToTimePoint("2024-01-02T06:04:05") );
	}
}

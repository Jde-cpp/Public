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

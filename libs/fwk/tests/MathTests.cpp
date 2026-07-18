#include <jde/fwk/utils/mathUtils.h>

#define let const auto

namespace Jde::Tests{
	TEST( MathTests, StatisticsAllNegative ){
		let r = Math::Statistics( vector<double>{-3.0, -1.0, -2.0} );
		EXPECT_EQ( r.Max, -1.0 );
		EXPECT_EQ( r.Min, -3.0 );
		EXPECT_EQ( r.Average, -2.0 );
	}
}

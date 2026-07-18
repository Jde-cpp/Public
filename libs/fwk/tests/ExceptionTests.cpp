#include <jde/fwk/exceptions/Exception.h>

#define let const auto

namespace Jde::Tests{

	TEST( ExceptionTests, WhatBadSpecNoTerminate ){
		Exception e{ SRCE_CUR, ELogLevel::Debug, "{:x}", 10 };
		let what = string{ e.what() };
		EXPECT_TRUE( what.contains("10") ) << what;
	}

	TEST( ExceptionTests, WhatGoodSpec ){
		Exception e{ SRCE_CUR, ELogLevel::Debug, "code: {}", 10 };
		EXPECT_EQ( string{e.what()}, "code: 10" );
	}
}

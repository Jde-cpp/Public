#include <jde/fwk/exceptions/Exception.h>
#include <jde/fwk/exceptions/ExternalException.h>

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

	//drivers echo user input in their messages - sqlite3_errmsg( 'near "{": syntax error' ), mysql server_message, odbc diagnostics.
	TEST( ExceptionTests, ExternalMessageBraces ){
		ExternalException e{ "near \"{\": syntax error", "select {} from t", {ELogLevel::Debug} };
		EXPECT_EQ( string{e.what()}, "near \"{\": syntax error - select {} from t" );
	}

	//a balanced pair is the worse case: it would consume the description arg, leaving the ' - {}' suffix without one.
	TEST( ExceptionTests, ExternalMessageBalancedBraces ){
		ExternalException e{ "unrecognized token: {}", "insert into t values(?)", {ELogLevel::Debug, ELogTags::Exception, 2067} };
		EXPECT_EQ( string{e.what()}, "(813)unrecognized token: {} - insert into t values(?)" );
	}

	TEST( ExceptionTests, ExternalMessageBracesNoDescription ){
		ExternalException e{ "near \"}\": syntax error", {}, {ELogLevel::Debug} };
		EXPECT_EQ( string{e.what()}, "near \"}\": syntax error" );
	}
}

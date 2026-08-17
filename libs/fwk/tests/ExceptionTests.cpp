#include <jde/fwk/exceptions/Exception.h>
#include <jde/fwk/exceptions/CodeException.h>
#include <jde/fwk/exceptions/ExternalException.h>
#include <jde/fwk/exceptions/IOException.h>
#include <jde/fwk/log/MemoryLog.h>

#define let const auto

namespace Jde::Tests{
	//named apart from the other suites' test exceptions: they all share this namespace, and two different classes
	//under one name across TUs is an ODR violation nothing would report.
	struct TypedException final : Exception{
		TypedException( string what, SRCE )ι:Exception{ move(what), {ELogLevel::Debug}, sl }{}
		α Move()ι->up<Exception> override{ return mu<TypedException>( move(*this) ); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }
	};

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

	TEST( ExceptionTests, HttpStatusDefaults ){
		Exception e{ "boom", {ELogLevel::Debug} };
		EXPECT_EQ( 500u, e.HttpStatus() ); //unset resolves to internal server error.
		EXPECT_EQ( Exception::ECategory::Jde, e.Category() );
		EXPECT_EQ( 0u, e.CategoryCode() );
	}

	//a wire exception loses its concrete type - the stored status set by the reconstructor is all that keeps a 401 a 401, so it has to ride Move().
	TEST( ExceptionTests, HttpStatusSurvivesMove ){
		Exception e{ "denied", {ELogLevel::Debug} };
		e.SetHttpStatus( EHttpStatus::IAmATeapot );
		let moved = e.Move();
		EXPECT_EQ( EHttpStatus::IAmATeapot, moved->HttpStatus() );
	}

	//"preserves dynamic type, unlike a ctor which would slice" - the whole reason FromPtr exists, and its only
	//library caller is MySqlQueryAwait, so nothing in this suite reached it even indirectly.
	TEST( ExceptionTests, FromPtrKeepsType ){
		std::exception_ptr p;
		try{ throw TypedException{"typed"}; }catch( ... ){ p = std::current_exception(); }
		let e = Exception::FromPtr( p );
		ASSERT_NE( e, nullptr );
		EXPECT_NE( dynamic_cast<TypedException*>(e.get()), nullptr ) << "arrived sliced: " << e->what();
		EXPECT_EQ( string{e->what()}, "typed" );
	}

	//the other two branches wrap rather than adopt, and both raise the level to Critical - an exception nobody
	//modelled is a bug in the caller, not an expected outcome.
	TEST( ExceptionTests, FromPtrWrapsForeignExceptions ){
		std::exception_ptr plain;
		try{ throw std::runtime_error{"plain"}; }catch( ... ){ plain = std::current_exception(); }
		let wrapped = Exception::FromPtr( plain );
		ASSERT_NE( wrapped, nullptr );
		EXPECT_EQ( wrapped->Level(), ELogLevel::Critical );
		EXPECT_EQ( string{wrapped->what()}, "std::exception - plain" );

		std::exception_ptr unknown;
		try{ throw 42; }catch( ... ){ unknown = std::current_exception(); }
		let opaque = Exception::FromPtr( unknown );
		ASSERT_NE( opaque, nullptr );
		EXPECT_EQ( wrapped->Level(), ELogLevel::Critical );
		EXPECT_EQ( string{opaque->what()}, "unknown exception" );
	}

	//the adopting ctor: a plain runtime_error contributes only its what(), while an Exception hands over its whole
	//state through operator= - which is what keeps a status code alive across the conversion.
	TEST( ExceptionTests, WrapRuntimeError ){
		Exception plain{ std::runtime_error{"plain"}, {ELogLevel::Debug} };
		EXPECT_EQ( string{plain.what()}, "plain" );

		TypedException inner{ "inner" };
		inner.SetHttpStatus( EHttpStatus::Conflict );
		Exception adopted{ move(inner), {ELogLevel::Debug} };
		EXPECT_EQ( string{adopted.what()}, "inner" );
		EXPECT_EQ( adopted.HttpStatus(), EHttpStatus::Conflict ) << "the Exception branch adopts the source's state, not just its message";
	}

	//log once, wherever it happens: at construction when the level warrants it, otherwise at destruction.  Move
	//hands the decision to the target - a double line at every Move()/Throw() would be a visible regression.
	TEST( ExceptionTests, LogsOnce ){
		auto& logger = Logging::GetLogger<Logging::MemoryLog>();
		Logging::ClearMemory();
		constexpr auto message = "logs-once message";
		{
			Exception e{ message, {ELogLevel::Information} };
			let moved = e.Move();
			let movedAgain = moved->Move();
		}//all three destroyed here.
		EXPECT_EQ( logger.Find(message).size(), 1u ) << "the message logged once per Move, not once per object";
	}

	//Code() is lazy: unset until asked, then memoized, and derived from the *format* so the same message shares one
	//code whatever its arguments.  IOException::SetWhat guards on HasCode precisely because reading it sets it.
	TEST( ExceptionTests, CodeIsStableHash ){
		Exception a{ SRCE_CUR, ELogLevel::Debug, "stable code {}", 1 };
		Exception b{ SRCE_CUR, ELogLevel::Debug, "stable code {}", 2 };
		EXPECT_FALSE( a.HasCode() );
		EXPECT_EQ( a.Code(), b.Code() ) << "hashed over the format, not the arguments";
		EXPECT_TRUE( a.HasCode() ) << "Code() memoizes into _code";

		Exception other{ SRCE_CUR, ELogLevel::Debug, "a different format" };
		EXPECT_NE( other.Code(), a.Code() );

		Exception explicitCode{ "boom", {ELogLevel::Debug, ELogTags::Exception, 42u} };
		EXPECT_TRUE( explicitCode.HasCode() );
		EXPECT_EQ( explicitCode.Code(), 42u ) << "an explicit code must not be replaced by the hash";
	}

	TEST( ExceptionTests, PrependWhat ){
		Exception e{ SRCE_CUR, ELogLevel::Debug, "message {}", 1 };
		e.PrependWhat( "prefix: " );
		EXPECT_EQ( string{e.what()}, "prefix: message 1" ) << "the format is rendered first, then prefixed";
		EXPECT_TRUE( e.ClientDetail().empty() ) << "internals stay in what() unless a subclass opts in";
	}

	TEST( ExceptionTests, IOExceptionWhat ){
		IO::IOException coded{ fs::path{"/x/y.txt"}, 2u, "Could not open file" };
		let what = string{ coded.what() };
		EXPECT_TRUE( what.contains("y.txt") ) << what;
		EXPECT_TRUE( what.contains("Could not open file") ) << what;
		EXPECT_TRUE( what.contains(std::system_category().message(2)) ) << "the code's system message is spelled out for the reader: " << what;
		EXPECT_EQ( coded.Path(), fs::path{"/x/y.txt"} );

		IO::IOException uncoded{ fs::path{"/x/y.txt"}, "no code here" };
		EXPECT_EQ( string{uncoded.what()}, "no code here path='/x/y.txt'" ) << "no code - no system message, and Code() must not have been read";
	}

	TEST( ExceptionTests, CodeExceptionToString ){
		let code = std::error_code{ 2, std::system_category() };
		EXPECT_EQ( CodeException::ToString(code), Ƒ("{} - {}", code.category().name(), code.message()) );
		CodeException e{ code, ELogTags::Test, ELogLevel::Debug };
		EXPECT_EQ( e.Code(), 2u ) << "the error_code's value rides as the exception code";
		EXPECT_TRUE( string{e.what()}.contains(code.message()) ) << e.what();
	}
}

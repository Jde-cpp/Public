//ql-review3 #28: three ql sites narrowed an exception to a base type on its way out - InsertAwait::InsertFailure and
//PurgeAwait::After took theirs *by value*, and SelectAwait::await_resume rethrew `*p` through the up<runtime_error>'s static
//type.  Every Jde exception's runtime_error base is runtime_error{""}, so a sliced one arrives with an empty what() and the
//base 500, losing DBException's classification: a duplicate-key createUser looked like a server fault instead of a 409.
#include "gtest/gtest.h"
#include <jde/db/DBException.h>
#include "globals.h"

#define let const auto

namespace Jde::Access::Tests{
	//the write-up's own case: two users with the same natural key.
	TEST( ExceptionDetailTests, DuplicateInsertKeepsItsDbClassification ){
		let root = GetRoot();
		let target = "review28-duplicate";
		let existing = UserPK{ GetId(GetUser(target, root)) };
		let m = "mutation createUser( name:\""+string{target}+"\", target:\""+string{target}+"\", providerId:1 )";
		try{
			QL().QuerySync<jvalue>( m, {}, root );
			ADD_FAILURE() << "the duplicate was accepted";
		}
		catch( const Exception& e ){
			EXPECT_FALSE( string{e.what()}.empty() ) << "sliced to runtime_error{\"\"}";
			EXPECT_EQ( e.HttpStatus(), EHttpStatus::Conflict ) << e.what();//DBException::HttpStatus maps Duplicate -> 409.
			EXPECT_NE( dynamic_cast<const DB::DBException*>(&e), nullptr ) << "the dynamic type did not survive: " << e.what();
		}
		PurgeUser( existing, root );
	}
}

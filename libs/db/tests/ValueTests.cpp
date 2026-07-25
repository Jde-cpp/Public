#include <gtest/gtest.h>
#include <jde/db/Value.h>

namespace Jde::DB::Tests{
	//#22: Value::operator=(uint) must return Value& (was `auto` -> deduced Value, i.e. a by-value copy).
	TEST( ValueTests, AssignUIntReturnsRef ){
		static_assert( std::is_same_v<decltype(std::declval<Value&>() = uint{5}), Value&>, "operator=(uint) must return Value&, not a copy." );
		Value v; v = uint{7};
		EXPECT_EQ( v.ToUInt(), 7u );
	}

	//#21: Value::ToUInt on a negative Double must not be UB - go through signed _int, matching the integer cases' modular wrap.
	TEST( ValueTests, ToUIntNegativeDouble ){
		EXPECT_EQ( Value{-5.0}.ToUInt(), (uint)(_int)-5 ); //modular wrap, deterministic (was UB).
		EXPECT_EQ( Value{-5.0}.ToInt(), -5 );              //ToInt = (_int)ToUInt round-trips.
		EXPECT_EQ( Value{42.0}.ToUInt(), 42u );            //non-negative unchanged.
	}
}

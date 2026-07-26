#include <gtest/gtest.h>
#include <jde/db/Value.h>

namespace Jde::DB::Tests{
	//#22: Value::operator=(uint) must return Value& (was `auto` -> deduced Value, i.e. a by-value copy).
	TEST( ValueTests, AssignUIntReturnsRef ){
		static_assert( std::is_same_v<decltype(std::declval<Value&>() = uint{5}), Value&>, "operator=(uint) must return Value&, not a copy." );
		Value v; v = uint{7};
		EXPECT_EQ( v.ToUInt(), 7u );
	}

	TEST( ValueTests, Equality ){
		static_assert( std::equality_comparable<Value> );
		EXPECT_TRUE( Value{5}==Value{5} );
		EXPECT_FALSE( Value{5}==Value{6} );
		EXPECT_TRUE( Value{string{"a"}}==Value{string{"a"}} );
		EXPECT_FALSE( Value{string{"a"}}==Value{string{"b"}} );
		EXPECT_FALSE( Value{5}==Value{string{"5"}} ); //different alternatives never compare equal.
		EXPECT_TRUE( Value{}==Value{} );              //both null.
		EXPECT_FALSE( Value{}==Value{5} );
		EXPECT_TRUE( Value{5}!=Value{6} );            //rewritten from ==.
	}

	TEST( ValueTests, Get ){
		EXPECT_EQ( Value{uint32_t{7}}.Get<uint32_t>(), 7u );
		EXPECT_EQ( Value{42}.Get<_int>(), 42 );          //widening across alternatives still goes through get_number.
		EXPECT_DOUBLE_EQ( Value{1.5}.Get<double>(), 1.5 );
		EXPECT_TRUE( Value{true}.Get<uint32_t>()==1u );
		EXPECT_EQ( Value{string{"abc"}}.Get<string>(), "abc" );

		EXPECT_THROW( Value{42}.Get<string>(), Exception );              //a number is not a string - throws, no longer a compile error.
		EXPECT_THROW( Value{string{"abc"}}.Get<uint32_t>(), Exception ); //and a string is not a number.
		EXPECT_THROW( Value{}.Get<uint32_t>(), Exception );              //null.
		EXPECT_THROW( Value{}.Get<string>(), Exception );
	}

	//#21: Value::ToUInt on a negative Double must not be UB - go through signed _int, matching the integer cases' modular wrap.
	TEST( ValueTests, ToUIntNegativeDouble ){
		EXPECT_EQ( Value{-5.0}.ToUInt(), (uint)(_int)-5 ); //modular wrap, deterministic (was UB).
		EXPECT_EQ( Value{-5.0}.ToInt(), -5 );              //ToInt = (_int)ToUInt round-trips.
		EXPECT_EQ( Value{42.0}.ToUInt(), 42u );            //non-negative unchanged.
	}
}

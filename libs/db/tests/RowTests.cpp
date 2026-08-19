#include <gtest/gtest.h>
#include <jde/db/Row.h>

namespace Jde::DB::Tests{
	//#54: GetUIntOpt read with get_uint(), which demands the *exact* uint alternative - so on a cell holding int64 it
	//ASSERTed (log-only in every build) and handed back an engaged optional{0}.  Every other integer *Opt, and GetUInt
	//itself, convert across alternatives.  It matters because sqlite delivers every integer as int64 (SqliteRow.cpp's
	//sqlite3_int64) and MySQL's signed INT columns are int64 kind too, so the odd one out was the one that was wrong.
	TEST( RowTests, GetUIntOptConvertsLikeGetUInt ){
		const Row row{ vector<Value>{ Value{(_int)7}, Value{}, Value{(uint)9} } }; //int64 (what a driver yields), null, uint.

		ASSERT_TRUE( row.GetUIntOpt(0).has_value() );
		EXPECT_EQ( *row.GetUIntOpt(0), 7u ); //was 0.
		EXPECT_EQ( row.GetUInt(0), 7u );     //the non-Opt twin always converted - that gap is the finding.
		EXPECT_EQ( row.Get<uint>(0), 7u );

		EXPECT_FALSE( row.GetUIntOpt(1).has_value() ); //null stays nullopt.
		EXPECT_EQ( *row.GetUIntOpt(2), 9u );           //and the exact alternative still works.
	}

	//#58: the other 19 Row accessors had no coverage either.  The invariant worth pinning is the pairing: for a non-null
	//cell every Get*Opt must answer exactly what its Get* twin answers, and for a null cell nullopt - which is the shape
	//#54 broke in one place and could break again anywhere.
	TEST( RowTests, EveryOptAccessorMatchesItsTwin ){
		const Row row{ vector<Value>{ Value{(_int)42}, Value{(uint32_t)7}, Value{true}, Value{2.5}, Value{string{"s"}}, Value{} } };
		constexpr uint nul = 5;

		EXPECT_EQ( row.GetIntOpt(0).value_or(0), row.GetInt(0) );
		EXPECT_EQ( row.GetInt32Opt(0).value_or(0), row.GetInt32(0) );
		EXPECT_EQ( row.GetUIntOpt(0).value_or(0), row.GetUInt(0) );      //#54's case: an int64 cell read by a uint accessor.
		EXPECT_EQ( row.GetUInt32Opt(1).value_or(0), row.GetUInt32(1) );
		EXPECT_EQ( row.GetUInt8Opt(1).value_or(0), row.GetUInt8(1) );
		EXPECT_EQ( row.GetUInt16Opt(1).value_or(0), row.GetUInt16(1) );
		EXPECT_EQ( row.GetBitOpt(2).value_or(false), row.GetBit(2) );
		EXPECT_EQ( row.GetDoubleOpt(3).value_or(0), row.GetDouble(3) );
		EXPECT_EQ( row.GetOpt<string>(4).value_or(""), row.GetString(4) );
		EXPECT_EQ( row.Get<uint>(0), 42u );

		//the null cell: every Opt form answers nullopt, none of them guesses.
		EXPECT_TRUE( row.IsNull(nul) );
		EXPECT_FALSE( row.GetIntOpt(nul).has_value() );
		EXPECT_FALSE( row.GetInt32Opt(nul).has_value() );
		EXPECT_FALSE( row.GetUIntOpt(nul).has_value() );
		EXPECT_FALSE( row.GetUInt32Opt(nul).has_value() );
		EXPECT_FALSE( row.GetUInt8Opt(nul).has_value() );
		EXPECT_FALSE( row.GetUInt16Opt(nul).has_value() );
		EXPECT_FALSE( row.GetBitOpt(nul).has_value() );
		EXPECT_FALSE( row.GetDoubleOpt(nul).has_value() );
		EXPECT_FALSE( row.GetOpt<string>(nul).has_value() );
		EXPECT_EQ( row.Size(), 6u );
	}
}

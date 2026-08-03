//The in-memory half of QL::Filter - the predicate the subscription fan-out uses to decide whether a changed row
//still matches a live query.  ToWhereClause (the SQL half) needs a DB::View, so it is not covered here.
#include <gtest/gtest.h>
#include <jde/db/Value.h>
#include <jde/ql/types/FilterQL.h>
#include <jde/ql/types/Input.h>
#include <jde/ql/types/Parser.h>

#define let const auto

namespace Jde::QL::Tests{
	struct FilterInput final : Input{
		FilterInput( sv args )ε: Input{ Parser::ParseArgs(string{args}), ms<jobject>() }{}
		α JTableName()Ι->string override{ return "tests"; }
	};
	Ω filterOf( sv args )ε->Filter{ return FilterInput{args}.Filter(); }
	Ω filterValue( DB::EOperator op, jvalue v )ι->FilterValue{ return FilterValue{ op, move(v) }; }

	TEST( FilterTests, OperatorNamesRoundTrip ){
		using enum DB::EOperator;
		for( let op : {Equal, NotEqual, Regex, Glob, In, NotIn, Greater, GreaterOrEqual, Less, LessOrEqual, ElementMatch} )
			EXPECT_EQ( ToQLOperator(QL::ToString(op)), op ); //qualified: DB::ToString(EOperator) - the SQL spelling - is an equally good ADL match.
		EXPECT_EQ( QL::ToString(Equal), "eq" );
		EXPECT_EQ( QL::ToString(GreaterOrEqual), "gte" );
		EXPECT_EQ( ToQLOperator("notAnOperator"), Equal ); //unknown spellings degrade to equality.
	}

	//DB::Value{uint{}}, not {42u}: an `unsigned int` literal selects the variant's uint32_t alternative, which
	//Value::ToJson has no case for (libs/db/src/Value.cpp) - it logs "Unknown type(uint32)" and yields json null,
	//so every comparison below would silently fail for reasons that have nothing to do with the filters.
	TEST( FilterTests, EqualityOperators ){
		EXPECT_TRUE( filterValue(DB::EOperator::Equal, 42).Test(DB::Value{uint{42}}) );
		EXPECT_FALSE( filterValue(DB::EOperator::Equal, 42).Test(DB::Value{uint{43}}) );
		EXPECT_TRUE( filterValue(DB::EOperator::NotEqual, 42).Test(DB::Value{uint{43}}) );
		EXPECT_FALSE( filterValue(DB::EOperator::NotEqual, 42).Test(DB::Value{uint{42}}) );
		EXPECT_TRUE( filterValue(DB::EOperator::Equal, "bob").Test(DB::Value{string{"bob"}}) );
		EXPECT_FALSE( filterValue(DB::EOperator::Equal, "bob").Test(DB::Value{string{"alice"}}) );
	}

	TEST( FilterTests, RelationalOperators ){
		EXPECT_TRUE( filterValue(DB::EOperator::Greater, 41).Test(DB::Value{uint{42}}) );
		EXPECT_FALSE( filterValue(DB::EOperator::Greater, 42).Test(DB::Value{uint{42}}) );
		EXPECT_TRUE( filterValue(DB::EOperator::GreaterOrEqual, 42).Test(DB::Value{uint{42}}) );
		EXPECT_TRUE( filterValue(DB::EOperator::Less, 43).Test(DB::Value{uint{42}}) );
		EXPECT_FALSE( filterValue(DB::EOperator::Less, 42).Test(DB::Value{uint{42}}) );
		EXPECT_TRUE( filterValue(DB::EOperator::LessOrEqual, 42).Test(DB::Value{uint{42}}) );
	}

	TEST( FilterTests, InAndNotIn ){
		let list = jarray{ 1, 2, 3 };
		EXPECT_TRUE( filterValue(DB::EOperator::In, list).Test(DB::Value{uint{2}}) );
		EXPECT_FALSE( filterValue(DB::EOperator::In, list).Test(DB::Value{uint{4}}) );
		EXPECT_TRUE( filterValue(DB::EOperator::NotIn, list).Test(DB::Value{uint{4}}) );
		EXPECT_FALSE( filterValue(DB::EOperator::NotIn, list).Test(DB::Value{uint{2}}) );
	}

	TEST( FilterTests, Regex ){
		EXPECT_TRUE( filterValue(DB::EOperator::Regex, "b.b").Test(DB::Value{string{"bob"}}) );
		EXPECT_FALSE( filterValue(DB::EOperator::Regex, "b.b").Test(DB::Value{string{"alice"}}) );
	}

	//Test() swallows its own exceptions and reports "no match" - an unparseable pattern must not escape.
	TEST( FilterTests, BadRegexDoesNotThrow ){
		bool matched{ true };
		EXPECT_NO_THROW( matched = filterValue(DB::EOperator::Regex, "[").Test(DB::Value{string{"bob"}}) );
		EXPECT_FALSE( matched );
	}

	TEST( FilterTests, TestAndIsBitwise ){
		EXPECT_TRUE( filterValue(DB::EOperator::Equal, 0x6).TestAnd(0x4) );
		EXPECT_FALSE( filterValue(DB::EOperator::Equal, 0x6).TestAnd(0x1) );
		EXPECT_FALSE( filterValue(DB::EOperator::Equal, "notANumber").TestAnd(0x4) );
	}

	//Input::Filter() turns args into per-column predicates: scalars are equality, arrays are In,
	//objects are one predicate per {operator: value} member.
	TEST( FilterTests, FilterFromArgsScalarIsEquality ){
		let f = filterOf( "{id: 42}" );
		EXPECT_FALSE( f.Empty() );
		EXPECT_TRUE( f.Test<uint>("id", 42u) );
		EXPECT_FALSE( f.Test<uint>("id", 43u) );
		EXPECT_TRUE( f.Test<uint>("other", 43u) ); //an unfiltered column always passes.
	}

	TEST( FilterTests, FilterFromArgsArrayIsIn ){
		let f = filterOf( R"({name: ["bob","alice"]})" );
		EXPECT_TRUE( f.Test<string>("name", "bob") );
		EXPECT_FALSE( f.Test<string>("name", "carol") );
	}

	//Multiple members on one column AND together: 1 < id < 10.
	TEST( FilterTests, FilterFromArgsObjectAndsOperators ){
		let f = filterOf( "{id: {gt: 1, lt: 10}}" );
		EXPECT_TRUE( f.Test<uint>("id", 5u) );
		EXPECT_FALSE( f.Test<uint>("id", 1u) );
		EXPECT_FALSE( f.Test<uint>("id", 10u) );
	}

	TEST( FilterTests, EmptyFilter ){
		EXPECT_TRUE( filterOf("{}").Empty() );
		EXPECT_TRUE( filterOf("{}").Test<uint>("id", 42u) );
	}

	TEST( FilterTests, TestF ){
		let f = filterOf( "{id: 42}" );
		bool evaluated{};
		EXPECT_TRUE( f.TestF<uint>("id", [&]{ evaluated = true; return 42u; }) );
		EXPECT_TRUE( evaluated );
		evaluated = false;
		EXPECT_TRUE( f.TestF<uint>("unfiltered", [&]{ evaluated = true; return 42u; }) );
		EXPECT_FALSE( evaluated ); //no filter on the column -> the value is never computed.
	}

	//FilterValue::ToString uses DB::ToString - the SQL spelling - not the "eq"/"gte" QL one.
	TEST( FilterTests, ToString ){
		EXPECT_EQ( filterOf("{id: 42}").ToString("id"), "==42" );
		EXPECT_EQ( filterOf("{id: 42}").ToString("other"), "none" );
	}
}

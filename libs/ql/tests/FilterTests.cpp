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

	TEST( FilterTests, UInt32ComparesLikeAnyOtherNumber ){
		EXPECT_TRUE( filterValue(DB::EOperator::Equal, 42).Test(DB::Value{uint32_t{42}}) );
		EXPECT_FALSE( filterValue(DB::EOperator::Equal, 42).Test(DB::Value{uint32_t{43}}) );
		EXPECT_TRUE( filterValue(DB::EOperator::Greater, 41).Test(DB::Value{uint32_t{42}}) );
		EXPECT_TRUE( filterValue(DB::EOperator::In, jarray{1,2,3}).Test(DB::Value{uint32_t{2}}) );
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
		EXPECT_TRUE( filterValue(DB::EOperator::Regex, "b.b").Test(DB::Value{string{"bib"}}) ); //the same compiled regex, reused - it is built in the ctor, not per Test().
	}

	//A pattern that can't be used matches nothing rather than throwing:  the ctor rejects it once instead of Test() throwing per row.
	TEST( FilterTests, UnusablePatternsMatchNothing ){
		bool matched{ true };
		EXPECT_NO_THROW( matched = filterValue(DB::EOperator::Regex, "[").Test(DB::Value{string{"bob"}}) ); //doesn't compile.
		EXPECT_FALSE( matched );
		EXPECT_FALSE( filterValue(DB::EOperator::Regex, 42).Test(DB::Value{string{"42"}}) );   //not a string.
		EXPECT_FALSE( filterValue(DB::EOperator::Glob, 42).Test(DB::Value{string{"42"}}) );
		let tooLong = string( MaxPatternLength+1, 'a' );
		EXPECT_FALSE( filterValue(DB::EOperator::Regex, jstring{tooLong}).Test(DB::Value{tooLong}) );
	}

	//Catastrophically-backtracking patterns must come back promptly instead of pinning the fan-out thread.  All of these
	//are valid, tiny, and far inside MaxPatternLength, so the compile-side guards do not see them - only the engine's
	//own bounds do, which is the whole reason the operator is on boost::regex rather than std::regex.  The first two
	//shapes are stopped by boost's recursion-depth guard, the `.*`-chains by BOOST_REGEX_MAX_STATE_COUNT.  On libc++,
	//whose std::regex is unbounded, this test does not finish.
	TEST( FilterTests, CatastrophicRegexIsBounded ){
		let subject = string( 42, 'a' )+"!"; //the trailing '!' denies the match, forcing every way to split the a's.
		for( let pattern : {"(a+)+$", "(a|a)*$", ".*.*.*.*.*.*=", "a*a*a*a*a*a*a*a*b"} ){
			let start = std::chrono::steady_clock::now();
			EXPECT_FALSE( filterValue(DB::EOperator::Regex, jstring{pattern}).Test(DB::Value{subject}) ) << pattern;
			EXPECT_LT( std::chrono::steady_clock::now()-start, std::chrono::seconds{5} ) << pattern << " was not bounded";
		}
	}

	//The bound is per match call, so the first abort poisons the pattern: one filter is tested against every log entry,
	//and SubscribeLog::Write would otherwise pay the whole budget - and it is a budget, not a hang - once per entry.
	TEST( FilterTests, BoundedRegexIsPoisonedAfterTheFirstAbort ){
		let subject = string( 42, 'a' )+"!";
		auto filter = filterValue( DB::EOperator::Regex, "(a+)+$" );
		EXPECT_FALSE( filter.Test(DB::Value{subject}) );
		let start = std::chrono::steady_clock::now();
		for( uint i{}; i<1000; ++i )
			EXPECT_FALSE( filter.Test(DB::Value{subject}) );
		EXPECT_LT( std::chrono::steady_clock::now()-start, std::chrono::milliseconds{100} ) << "1000 rows re-ran the aborted match instead of short-circuiting";
	}

	//The cap has to leave ordinary patterns room: a linear match over a long subject costs states too, and a filter that
	//starts returning false under load would be a far worse bug than the one the cap fixes.
	TEST( FilterTests, StateCapLeavesHeadroomForOrdinaryPatterns ){
		let haystack = string( 4000, 'x' )+"needle"+string( 4000, 'y' );
		EXPECT_TRUE( filterValue(DB::EOperator::Regex, ".*needle.*").Test(DB::Value{haystack}) );
		EXPECT_FALSE( filterValue(DB::EOperator::Regex, ".*absent.*").Test(DB::Value{haystack}) );
	}

	//glob is a glob, not a regex.  `*abc*` is not even a legal regex (nothing for the leading '*' to repeat), so this used
	//to throw inside Test() and report "no match" for everything.
	TEST( FilterTests, Glob ){
		let glob = []( sv pattern, sv value ){ return filterValue(DB::EOperator::Glob, jstring{pattern}).Test( DB::Value{string{value}} ); };
		EXPECT_TRUE( glob("*abc*", "xxabcyy") );
		EXPECT_TRUE( glob("*abc*", "abc") );
		EXPECT_FALSE( glob("*abc*", "xxabyy") );
		EXPECT_TRUE( glob("abc*", "abcdef") );
		EXPECT_FALSE( glob("abc*", "xabcdef") );
		EXPECT_TRUE( glob("*def", "abcdef") );
		EXPECT_TRUE( glob("a*b*c", "axxbyyc") );  //two stars, each backtracking independently.
		EXPECT_TRUE( glob("b?b", "bob") );
		EXPECT_FALSE( glob("b?b", "boob") );
		EXPECT_TRUE( glob("bob", "bob") );
		EXPECT_FALSE( glob("bob", "bobby") );
		EXPECT_TRUE( glob("*", "anything") );
		EXPECT_TRUE( glob("*", "") );
		EXPECT_FALSE( glob("", "notEmpty") );
		EXPECT_TRUE( glob("", "") );
	}

	//The metacharacters are the glob ones - a regex's are literal, which is the whole point of the operator being distinct.
	TEST( FilterTests, GlobMetacharactersAreLiteral ){
		let glob = []( sv pattern, sv value ){ return filterValue(DB::EOperator::Glob, jstring{pattern}).Test( DB::Value{string{value}} ); };
		EXPECT_TRUE( glob("a.c", "a.c") );
		EXPECT_FALSE( glob("a.c", "abc") ); //'.' is not "any character" here.
		EXPECT_TRUE( glob("a+b", "a+b") );
		EXPECT_TRUE( glob("(x)", "(x)") );
	}

	TEST( FilterTests, GlobCharacterClasses ){
		let glob = []( sv pattern, sv value ){ return filterValue(DB::EOperator::Glob, jstring{pattern}).Test( DB::Value{string{value}} ); };
		EXPECT_TRUE( glob("[bB]ob", "Bob") );
		EXPECT_TRUE( glob("[bB]ob", "bob") );
		EXPECT_FALSE( glob("[bB]ob", "rob") );
		EXPECT_TRUE( glob("[a-c]at", "bat") );
		EXPECT_FALSE( glob("[a-c]at", "hat") );
		EXPECT_TRUE( glob("[^a-c]at", "hat") );  //sqlite spells negation '^'…
		EXPECT_FALSE( glob("[!a-c]at", "bat") ); //…and '!' is accepted too.
		EXPECT_TRUE( glob("[]x]y", "]y") );      //a ']' first is a literal, not the terminator.
		EXPECT_TRUE( glob("[abc", "[abc") );     //unterminated -> the '[' is a literal.
		EXPECT_TRUE( glob("*[0-9]", "port 8") );
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

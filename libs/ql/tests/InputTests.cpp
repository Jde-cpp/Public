//Input is the arg/variable half of every query and mutation.  It is abstract only in JTableName(), so a one-line
//stub gives full coverage of the accessors, variable extrapolation, paging and orderBy without a schema.
#include <gtest/gtest.h>
#include <jde/ql/types/Input.h>
#include <jde/ql/types/Parser.h>

#define let const auto

namespace Jde::QL::Tests{
	struct TestInput final : Input{
		TestInput( sv args, jobject variables={} )ε:
			Input{ args.empty() ? jobject{} : Parser::ParseArgs(string{args}), ms<jobject>(move(variables)) }{}
		α JTableName()Ι->string override{ return "tests"; }
	};

	TEST( InputTests, FindAndAs ){
		TestInput in{ R"({id: 42, name: "bob", active: true, tags: ["a"], nested: {x: 1}})" };
		EXPECT_EQ( in.AsNumber<uint>("id"), 42u );
		EXPECT_EQ( in.As<jstring>("name"), "bob" );
		EXPECT_EQ( in.Find<bool>("active"), true );
		EXPECT_NE( in.FindPtr<jarray>("tags"), nullptr );
		EXPECT_NE( in.FindPtr<jobject>("nested"), nullptr );
		EXPECT_EQ( in.FindPtr<jstring>("id"), nullptr );  //wrong kind -> null, not a throw.
		EXPECT_EQ( in.TryNumber<uint>("name"), nullopt ); //ditto.
		EXPECT_THROW( in.As<jstring>("missing"), Exception );
		EXPECT_THROW( in.AsNumber<uint>("missing"), Exception );
	}

	TEST( InputTests, VariablesResolveThroughAccessors ){
		TestInput in{ R"({id: $userId, name: $userName})", jobject{ {"userId",42}, {"userName","bob"} } };
		EXPECT_EQ( in.AsNumber<uint>("id"), 42u );
		EXPECT_EQ( in.As<jstring>("name"), "bob" );
		EXPECT_EQ( in.Id<uint>(), 42u );
	}

	TEST( InputTests, ExtrapolateVariablesRecurses ){
		TestInput in{ R"({id: $userId, in: {gt: $low}, list: [$a, 7, {b: $b}], plain: "x"})",
			jobject{ {"userId",42}, {"low",1}, {"a","first"}, {"b",true} } };
		let args = in.ExtrapolateVariables();
		EXPECT_EQ( args.at("id").to_number<uint>(), 42u );
		EXPECT_EQ( args.at("in").as_object().at("gt").to_number<uint>(), 1u ); //nested object.
		let& list = args.at("list").as_array();
		ASSERT_EQ( list.size(), 3u );
		EXPECT_EQ( list[0].as_string(), "first" );                             //array element.
		EXPECT_EQ( list[1].to_number<uint>(), 7u );                            //non-variable passes through.
		EXPECT_TRUE( list[2].as_object().at("b").as_bool() );                  //object inside an array.
		EXPECT_EQ( args.at("plain").as_string(), "x" );
	}

	TEST( InputTests, UnknownVariableExtrapolatesToNull ){
		TestInput in{ R"({id: $missing})" };
		EXPECT_TRUE( in.ExtrapolateVariables().at("id").is_null() );
	}

	//Input.h reads paging from "offset", falling back to the legacy "skip" spelling.
	TEST( InputTests, LimitAndOffset ){
		EXPECT_EQ( TestInput{"{limit: 10, offset: 5}"}.Limit(), 10u );
		EXPECT_EQ( TestInput{"{limit: 10, offset: 5}"}.Offset(), 5u );
		EXPECT_EQ( TestInput{"{skip: 7}"}.Offset(), 7u );            //legacy spelling.
		EXPECT_EQ( TestInput{"{offset: 3, skip: 7}"}.Offset(), 3u ); //offset wins.
		EXPECT_EQ( TestInput{""}.Limit(), 0u );
		EXPECT_EQ( TestInput{""}.Offset(), 0u );
	}

	TEST( InputTests, FindKey ){
		EXPECT_TRUE( TestInput{"{id: 42}"}.FindKey()->IsPK() );
		EXPECT_EQ( TestInput{"{id: 42}"}.GetKey().PK(), 42u );
		EXPECT_FALSE( TestInput{R"({target: "bob"})"}.FindKey()->IsPK() );
		EXPECT_EQ( TestInput{R"({target: "bob"})"}.GetKey().NK(), "bob" );
		EXPECT_EQ( TestInput{R"({id: 42, target: "bob"})"}.GetKey().PK(), 42u ); //id wins.
		EXPECT_EQ( TestInput{R"({name: "bob"})"}.FindKey(), nullopt );
		EXPECT_THROW( TestInput{R"({name: "bob"})"}.GetKey(), Exception );
	}

	//OrderByJson caches into the Input, so the Input has to outlive the returned reference - keep each one named.
	TEST( InputTests, OrderByJson ){
		{ //bare string -> ascending.
			TestInput in{ R"({orderBy: "name"})" };
			let& o = in.OrderByJson();
			ASSERT_EQ( o.size(), 1u );
			EXPECT_EQ( o[0].first, "name" );
			EXPECT_TRUE( o[0].second );
		}
		{ //Angular Material's {active,direction} sort shape.
			TestInput in{ R"({orderBy: {active: "name", direction: "desc"}})" };
			let& o = in.OrderByJson();
			ASSERT_EQ( o.size(), 1u );
			EXPECT_EQ( o[0].first, "name" );
			EXPECT_FALSE( o[0].second );
		}
		{ //{column: direction} pairs.
			TestInput in{ R"({orderBy: {name: "desc", id: "asc"}})" };
			let& o = in.OrderByJson();
			ASSERT_EQ( o.size(), 2u );
			EXPECT_FALSE( o[0].second );
			EXPECT_TRUE( o[1].second );
		}
		TestInput noOrderBy{ "{id: 42}" };
		EXPECT_TRUE( noOrderBy.OrderByJson().empty() );
	}

	TEST( InputTests, ArgStringResolvesVariables ){
		EXPECT_EQ( TestInput{""}.ArgString(), "" );
		EXPECT_EQ( TestInput{"{id: 42}"}.ArgString(), "(id: 42)" );
		EXPECT_EQ( TestInput{R"({name: "bob"})"}.ArgString(), R"((name: "bob"))" );
		TestInput in{ R"({name: $userName})", jobject{ {"userName","bob"} } };
		EXPECT_EQ( in.ArgString(), R"((name: "bob"))" );
	}
}

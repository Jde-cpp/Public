//Tokenizer + argument parsing.  Everything here is schema-free: Parser::Next/Peek/Trim and the static
//Parser::ParseArgs are pure string->string/json, so no AppSchema or data source is needed.
#include <gtest/gtest.h>
#include <jde/ql/types/Parser.h>

#define let const auto

namespace Jde::QL::Tests{
	Ω parser( string text, sv delimiters="{}()," )ι->Parser{ return Parser{ move(text), delimiters }; }

	TEST( ParserTests, NextSplitsOnWhitespaceAndDelimiters ){
		auto p = parser( "users( id: 42 ){ name }" );
		EXPECT_EQ( p.Next(), "users" );
		EXPECT_EQ( p.Next(), "(" );   //a delimiter hit at the token start is returned as a 1-char token.
		EXPECT_EQ( p.Next(), "id:" ); //':' is not a delimiter - it stays glued to the name.
		EXPECT_EQ( p.Next(), "42" );
		EXPECT_EQ( p.Next(), ")" );
		EXPECT_EQ( p.Next(), "{" );
		EXPECT_EQ( p.Next(), "name" );
		EXPECT_EQ( p.Next(), "}" );
		EXPECT_EQ( p.Next(), "" );    //exhausted.
	}

	TEST( ParserTests, PeekDoesNotConsume ){
		auto p = parser( "users { id }" );
		EXPECT_EQ( p.Peek(), "users" );
		EXPECT_EQ( p.Peek(), "users" ); //idempotent.
		EXPECT_EQ( p.Next(), "users" );
		EXPECT_EQ( p.Next(), "{" );
	}

	TEST( ParserTests, NextToCharReturnsThroughTerminator ){
		auto p = parser( "( id: 42 ) { name }" );
		EXPECT_EQ( p.Next(')'), "( id: 42 )" );
		EXPECT_EQ( p.Next(), "{" );
	}

	//Next(char) has to walk over quoted strings so a terminator inside a literal doesn't end the token early.
	TEST( ParserTests, NextToCharSkipsQuotedTerminator ){
		auto p = parser( R"((name: "a)b") rest)" );
		EXPECT_EQ( p.Next(')'), R"((name: "a)b"))" );
	}

	TEST( ParserTests, NextToCharUnterminatedQuoteThrows ){
		auto p = parser( R"((name: "abc)" );
		EXPECT_THROW( p.Next(')'), Exception );
	}

	TEST( ParserTests, TrimStripsLeadingTokenAndOuterBraces ){
		auto p = parser( "query { users { id } }" );
		EXPECT_TRUE( p.Trim("query") );
		EXPECT_EQ( p.Next(), "users" ); //the outer { } went with the trim.
		EXPECT_FALSE( parser("users { id }").Trim("query") );
	}

	//Trim() is called after Peek() in QL::Parse - the peeked token must not be re-emitted afterwards.
	TEST( ParserTests, TrimAfterPeekResetsPeek ){
		auto p = parser( "query { users { id } }" );
		EXPECT_EQ( p.Peek(), "query" );
		EXPECT_TRUE( p.Trim("query") );
		EXPECT_EQ( p.Peek(), "users" );
	}

	TEST( ParserTests, ParseArgsQuotesBareKeys ){
		let args = Parser::ParseArgs( R"({id: 42, name: "bob", active: true, missing: null, ratio: -1.5})" );
		EXPECT_EQ( args.at("id").to_number<uint>(), 42u );
		EXPECT_EQ( args.at("name").as_string(), "bob" );
		EXPECT_TRUE( args.at("active").as_bool() );
		EXPECT_TRUE( args.at("missing").is_null() );
		EXPECT_DOUBLE_EQ( args.at("ratio").to_number<double>(), -1.5 );
	}

	//$name becomes the escaped string "\b$name"; Input::ExtrapolateVariables resolves it later.
	TEST( ParserTests, ParseArgsEncodesVariables ){
		let args = Parser::ParseArgs( R"({id: $userId})" );
		EXPECT_EQ( args.at("id").as_string(), string{Input::Escape}+"userId" );
	}

	TEST( ParserTests, ParseArgsNestedObjectsAndArrays ){
		let args = Parser::ParseArgs( R"({id: {in: [1,2,3]}, names: ["a","b"], nested: {a: {b: "c"}}})" );
		let& in = args.at("id").as_object().at("in").as_array();
		ASSERT_EQ( in.size(), 3u );
		EXPECT_EQ( in[2].to_number<uint>(), 3u );
		EXPECT_EQ( args.at("names").as_array().size(), 2u );
		EXPECT_EQ( args.at("nested").as_object().at("a").as_object().at("b").as_string(), "c" );
	}

	TEST( ParserTests, ParseArgsKeepsEscapedQuoteInString ){
		let args = Parser::ParseArgs( R"({name: "a\"b"})" );
		EXPECT_EQ( args.at("name").as_string(), R"(a"b)" );
	}

	TEST( ParserTests, ParseArgsEmptyObject ){
		EXPECT_TRUE( Parser::ParseArgs("{}").empty() );
	}

	TEST( ParserTests, ParseArgsRejectsMalformed ){
		EXPECT_THROW( Parser::ParseArgs("{id: 42"), Exception );      //unterminated object.
		EXPECT_THROW( Parser::ParseArgs(R"({active: tru})"), Exception ); //'t' must spell true.
		EXPECT_THROW( Parser::ParseArgs("{id: @}"), Exception );      //unexpected character.
	}

	TEST( MutationQLTests, IsMutation ){
		EXPECT_TRUE( MutationQL::IsMutation("mutation") );
		EXPECT_TRUE( MutationQL::IsMutation("createUser") );
		EXPECT_TRUE( MutationQL::IsMutation("purgeGroup") );
		EXPECT_FALSE( MutationQL::IsMutation("users") );
		EXPECT_FALSE( MutationQL::IsMutation("query") ); //explicitly not a mutation, even though it starts with no verb.
	}

	TEST( MutationQLTests, ParseCommand ){
		let [name,type] = MutationQL::ParseCommand( "createUser" );
		EXPECT_EQ( name, "user" ); //verb stripped, first letter lowered.
		EXPECT_EQ( type, EMutationQL::Create );
		EXPECT_EQ( get<1>(MutationQL::ParseCommand("removeGroupMember")), EMutationQL::Remove );
		EXPECT_THROW( MutationQL::ParseCommand("frobnicateUser"), Exception );
	}

	TEST( MutationQLTests, ToStringRoundTripsArgs ){
		MutationQL m{ "createUser", Parser::ParseArgs(R"({target: "bob"})"), ms<jobject>(), optional<TableQL>{}, true, vector<sp<DB::AppSchema>>{}, true };
		EXPECT_EQ( m.ToString(), R"(createUser("target":"bob"))" );
		EXPECT_EQ( m.JTableName(), "user" );
	}
}

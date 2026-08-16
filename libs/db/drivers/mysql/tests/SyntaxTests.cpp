#include <gtest/gtest.h>
#include <jde/db/Value.h>
#include "../src/MySqlSyntax.h"

namespace Jde::DB::MySql::Tests{
	//Pure dialect assertions - no connection, no server.  Moved out of the sqlite driver's SchemaTests, which could
	//only reach MySqlSyntax while it still lived in Jde.DB.
	TEST( SyntaxTests, MySqlDialect ){
		const auto& my = MySqlSyntax::Instance();

		//every unsigned width must carry 'unsigned' (sqlite maps them all to 'integer', so it could not exercise this).
		EXPECT_EQ( my.ToString(EType::UInt), "int unsigned" );
		EXPECT_EQ( my.ToString(EType::ULong), "bigint(20) unsigned" );
		EXPECT_EQ( my.ToString(EType::UInt16), "smallint unsigned" ); //#6: was "smallint" - uint16 columns got half the range.
		EXPECT_EQ( my.ToString(EType::UInt8), "tinyint unsigned" );
		EXPECT_EQ( my.ToString(EType::Int16), "smallint" ); //signed stays plain.

		EXPECT_TRUE( my.HasUnsigned() );
		EXPECT_FALSE( my.HasCatalogs() );
		EXPECT_TRUE( my.HasProcs() );
		EXPECT_FALSE( my.NeedsIdentityInsert() );
		EXPECT_EQ( my.IdentityColumnSyntax(), "AUTO_INCREMENT" );
		EXPECT_EQ( my.IdentitySelect(), "LAST_INSERT_ID()" );
		EXPECT_EQ( my.SysSchema(), "sys" );
		EXPECT_EQ( my.GuidType(), "binary" );
		EXPECT_EQ( my.NowDefault(), "CURRENT_TIMESTAMP" );
	}

	TEST( SyntaxTests, EscapeDdl ){
		EXPECT_EQ( MySqlSyntax::Instance().EscapeDdl("t"), "`t`" );          //backticks, where the base dialect uses [brackets].
		EXPECT_EQ( MySqlSyntax::Instance().EscapeDdl("s.t"), "`s`.`t`" );    //each part quoted separately.
	}

	//SqlName() quotes a reserved table name in generated sql - `groups` is reserved in mysql 8+ but fine with a prefix or in the base dialect.
	TEST( SyntaxTests, IsReservedWord ){
		EXPECT_TRUE( MySqlSyntax::Instance().IsReservedWord("groups") );
		EXPECT_FALSE( MySqlSyntax::Instance().IsReservedWord("access_groups") );
		EXPECT_FALSE( Syntax::Instance().IsReservedWord("groups") );
	}

	//MySQL is the only dialect with a regex engine, so `regex:` passes through; `glob:` becomes an anchored REGEXP rather
	//than a LIKE, because LIKE would drop the character classes.
	TEST( SyntaxTests, PatternOperators ){
		const auto& my = MySqlSyntax::Instance();
		EXPECT_EQ( my.PatternOperator(EOperator::Regex), "regexp" );
		EXPECT_EQ( my.PatternOperator(EOperator::Glob), "regexp" );
		EXPECT_THROW( my.PatternOperator(EOperator::ElementMatch), Exception );

		EXPECT_EQ( my.PatternParam(EOperator::Regex, "b.b"), "b.b" ); //verbatim - it is already a regex.
		EXPECT_EQ( my.PatternParam(EOperator::Glob, "*abc*"), "^.*abc.*$" );
		EXPECT_EQ( my.PatternParam(EOperator::Glob, "b?b"), "^b.b$" );
	}

	//glob -> regex is a total mapping, which is why it beats glob -> LIKE here.  Anchored, because glob matches the whole
	//value where REGEXP is a search.
	TEST( SyntaxTests, GlobToRegex ){
		EXPECT_EQ( GlobToRegex("*abc*"), "^.*abc.*$" );
		EXPECT_EQ( GlobToRegex("b?b"), "^b.b$" );
		EXPECT_EQ( GlobToRegex("bob"), "^bob$" );        //anchored: a bare name must not match "bobby".
		EXPECT_EQ( GlobToRegex("a.c"), "^a\\.c$" );      //'.' is literal in a glob - it must be escaped, not left as "any".
		EXPECT_EQ( GlobToRegex("a+b"), "^a\\+b$" );
		EXPECT_EQ( GlobToRegex("(x)"), "^\\(x\\)$" );
		EXPECT_EQ( GlobToRegex("[a-c]at"), "^[a-c]at$" ); //classes are spelled the same in both languages.
		EXPECT_EQ( GlobToRegex("[!a-c]at"), "^[^a-c]at$" );//glob's '!' negation becomes regex '^'.
		EXPECT_EQ( GlobToRegex("[^a-c]at"), "^[^a-c]at$" );
		EXPECT_EQ( GlobToRegex("[abc"), "^\\[abc$" );     //unterminated class -> a literal '[', as in sqlite.
	}

	TEST( SyntaxTests, Limit ){
		const auto& my = MySqlSyntax::Instance();
		const auto sql = string{ "select * from t" };
		EXPECT_EQ( my.Limit(sql, 10, 0), "select * from t limit 10" ); //limit-only.
		EXPECT_EQ( my.Limit(sql, 10, 5), "select * from t limit 10 offset 5" ); //limit + skip.
		//OFFSET requires a LIMIT, so skip-only sends 2^64-1 ("all rows") rather than 'limit 0', which returns none.
		EXPECT_EQ( my.Limit(sql, 0, 5), "select * from t limit 18446744073709551615 offset 5" );
	}
}

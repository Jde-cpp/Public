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

	TEST( SyntaxTests, Limit ){
		const auto& my = MySqlSyntax::Instance();
		const auto sql = string{ "select * from t" };
		EXPECT_EQ( my.Limit(sql, 10, 0), "select * from t limit 10" ); //limit-only.
		EXPECT_EQ( my.Limit(sql, 10, 5), "select * from t limit 10 offset 5" ); //limit + skip.
		//OFFSET requires a LIMIT, so skip-only sends 2^64-1 ("all rows") rather than 'limit 0', which returns none.
		EXPECT_EQ( my.Limit(sql, 0, 5), "select * from t limit 18446744073709551615 offset 5" );
	}
}

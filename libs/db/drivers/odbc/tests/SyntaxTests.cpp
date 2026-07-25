#include <gtest/gtest.h>
#include <jde/db/Value.h>
#include <jde/db/generators/Syntax.h>

namespace Jde::DB::Odbc::Tests{
	//There is no MsSqlSyntax type - the base DB::Syntax *is* the SQL Server dialect, and OdbcDataSource returns it
	//unchanged.  These pin that dialect the way the mysql driver's SyntaxTests pin theirs; no connection is opened.
	TEST( SyntaxTests, SqlServerDialect ){
		const auto& s = Syntax::Instance();
		EXPECT_EQ( s.IdentityColumnSyntax(), "identity(1001,1)" );
		EXPECT_EQ( s.IdentitySelect(), "@@identity" );
		EXPECT_EQ( s.SysSchema(), "dbo" );
		EXPECT_EQ( s.GuidType(), "uniqueidentifier" );
		EXPECT_EQ( s.NowDefault(), "getutcdate()" );
		EXPECT_EQ( s.ProcParameterPrefix(), "@" );

		EXPECT_TRUE( s.HasSchemas() );
		EXPECT_TRUE( s.HasCatalogs() );
		EXPECT_TRUE( s.HasProcs() );
		EXPECT_TRUE( s.CanAddForeignKeys() );
		EXPECT_TRUE( s.NeedsIdentityInsert() );
		EXPECT_FALSE( s.HasUnsigned() ); //no unsigned integers - uint columns fall back to the signed width.
		EXPECT_EQ( s.ToString(EType::UInt), "int" );
		EXPECT_EQ( s.ToString(EType::ULong), "bigint" );
	}

	TEST( SyntaxTests, EscapeDdl ){
		EXPECT_EQ( Syntax::Instance().EscapeDdl("t"), "[t]" );        //bracket quoting, where mysql uses `backticks`.
		EXPECT_EQ( Syntax::Instance().EscapeDdl("s.t"), "[s].[t]" );  //each part quoted separately.
	}

	TEST( SyntaxTests, Limit ){
		const auto& s = Syntax::Instance();
		const auto sql = string{ "select * from t order by id" };
		//T-SQL: OFFSET is mandatory before FETCH, so it is emitted even when skip is 0.
		EXPECT_EQ( s.Limit(sql, 10, 0), "select * from t order by id offset 0 rows fetch next 10 rows only" );
		EXPECT_EQ( s.Limit(sql, 10, 5), "select * from t order by id offset 5 rows fetch next 10 rows only" );
		EXPECT_EQ( s.Limit(sql, 0, 5), "select * from t order by id offset 5 rows" ); //skip-only: no FETCH clause.
	}
}

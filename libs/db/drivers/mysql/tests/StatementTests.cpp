#include <gtest/gtest.h>
#include "../src/MySqlStatements.h"

namespace Jde::DB::MySql::Tests{
	//Statement-text assertions only - MySqlStatements.cpp is compiled into this exe, nothing connects to a server.

	//#1: con was joined on CONSTRAINT_NAME alone.  Constraint names are unique per schema, not per server, so every
	//fk row fanned out across the identically-named constraints in the other schemas (debug/rls/*_access twins);
	//LoadForeignKeys folded the copies into Columns as {col,col}, which SyncFKs' `Columns==vector{column->Name}`
	//compare never matches - so every sync run re-created every foreign key under a bumped name.
	TEST( StatementTests, ForeignKeySqlSchemaQualifiesConJoins ){
		const auto sql = Ddl::ForeignKeySql( true );
		EXPECT_NE( sql.find("con.CONSTRAINT_SCHEMA=fk.CONSTRAINT_SCHEMA"), string::npos );
		EXPECT_NE( sql.find("con.TABLE_NAME=fk.TABLE_NAME"), string::npos );
		EXPECT_NE( sql.find("pk.CONSTRAINT_SCHEMA COLLATE utf8_general_ci=con.UNIQUE_CONSTRAINT_SCHEMA"), string::npos );
		//the schema-qualified joins hold whether or not the caller adds the where filters.
		EXPECT_NE( Ddl::ForeignKeySql(false).find("con.CONSTRAINT_SCHEMA=fk.CONSTRAINT_SCHEMA"), string::npos );
	}

	TEST( StatementTests, ForeignKeySqlSchemaParams ){
		const auto sql = Ddl::ForeignKeySql( true );
		EXPECT_NE( sql.find("where pk.TABLE_SCHEMA=?"), string::npos );//2 params, in the order LoadForeignKeys binds them.
		EXPECT_NE( sql.find("and fk.TABLE_SCHEMA=?"), string::npos );
		EXPECT_EQ( std::ranges::count(sql, '?'), 2 );
		EXPECT_EQ( std::ranges::count(Ddl::ForeignKeySql(false), '?'), 0 );
	}
}

#include <gtest/gtest.h>
#include <jde/db/db.h>
#include <jde/db/generators/InsertClause.h>
#include <jde/db/generators/Object.h>
#include <jde/db/generators/Sql.h>
#include <jde/db/meta/AppSchema.h>

namespace Jde::DB::Tests{
	//#10: SchemaDdl::Sync now delegates to ObjectPrefix (was a swapped-ternary duplicate that yielded "" for a "db.um_" prefix). Lock in ObjectPrefix.
	TEST( AppSchemaTests, ObjectPrefix ){
		EXPECT_EQ( (AppSchema{ "s", {}, "db.um_" }).ObjectPrefix(), "um_" ); //MSSQL-style schema.prefix -> the object prefix after the dot.
		EXPECT_EQ( (AppSchema{ "s", {}, "um_" }).ObjectPrefix(), "um_" );    //no dot -> unchanged.
		EXPECT_EQ( (AppSchema{ "s", {}, "" }).ObjectPrefix(), "" );          //empty -> empty.
	}

	//#26: InsertClause::Proc placeholder count must match the params (was a leading "(?" -> zero Values gave `name(?)` with 0 params), and SequenceColumn must not deref a null column.
	TEST( InsertClauseTests, ProcParamCountMatches ){
		EXPECT_EQ( InsertClause( "p", vector<Value>{} ).Move().Text, "p()" );          //0 values -> no placeholder (was "p(?)").
		EXPECT_TRUE( InsertClause( "p", vector<Value>{} ).Move().Params.empty() );
		auto sql = InsertClause( "p", vector<Value>{ Value{1}, Value{2} } ).Move();
		EXPECT_EQ( sql.Text, "p(?,?)" );
		EXPECT_EQ( sql.Params.size(), 2u );
	}
	TEST( InsertClauseTests, SequenceColumnNullColumnSafe ){
		InsertClause ins{ "p", vector<Value>{ Value{1} } }; //proc ctor -> Values have null columns.
		EXPECT_EQ( ins.SequenceColumn(), nullptr );          //was a null-column ->Table deref (segfault).
	}

	//#25: an Object holding Values must render placeholders (?,?), not the literal `[ 1, 2]` - GetParams appends the values, so the counts must match.
	TEST( ObjectTests, ValuesRendersPlaceholders ){
		DB::Object o = vector<DB::Value>{ DB::Value{1}, DB::Value{2}, DB::Value{3} };
		EXPECT_EQ( DB::ToString(o), "(?,?,?)" );
		EXPECT_EQ( DB::GetParams(o).size(), 3u ); //placeholder count now matches param count.
	}
}

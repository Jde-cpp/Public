#include <gtest/gtest.h>
#include <jde/db/db.h>
#include <jde/db/meta/Column.h>
#include "../../../src/meta/IServerMeta.h"
#include "../../../src/meta/ddl/TableDdl.h" //complete type for LoadTable's sp<TableDdl>.
#include "../../../src/meta/ddl/Index.h" //complete type for LoadIndexes' vector<Index>.
#include "../../../src/meta/ddl/ForeignKey.h" //complete type for LoadForeignKeys' flat_map<string,ForeignKey>.
#include "../src/SqliteSyntax.h"
#include <jde/app/AppQL.h>
#include <jde/db/generators/FromClause.h>
#include <jde/db/generators/Object.h>
#include <jde/db/generators/InsertClause.h>
#include <jde/db/generators/Sql.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Cluster.h>
#include <jde/access/Authorize.h>
#include <jde/db/DBException.h>
#include <jde/fwk/log/MemoryLog.h>
#include <jde/fwk/log/Entry.h>

#define let const auto

namespace Jde::DB::Sqlite::Tests{
	struct SchemaTests : BackendTests{};
	INSTANTIATE_BACKENDS( SchemaTests );

	TEST_P( SchemaTests, ServerMetaLoadTable ){
		let table = _ds->ServerMeta().LoadTable( "main", "access_identities" );
		ASSERT_TRUE( table );
		ASSERT_EQ( table->Columns.size(), 10u );

		let id = table->FindColumn( "identity_id" );
		ASSERT_TRUE( id );
		EXPECT_TRUE( id->IsSequence ); //single-column integer pk = rowid alias.
		EXPECT_FALSE( id->IsNullable );

		let name = table->FindColumn( "name" );
		ASSERT_TRUE( name );
		EXPECT_EQ( name->Type, EType::VarChar );

		let created = table->FindColumn( "created" );
		ASSERT_TRUE( created );
		EXPECT_EQ( created->Type, EType::DateTime );

		let description = table->FindColumn( "description" );
		ASSERT_TRUE( description );
		EXPECT_TRUE( description->IsNullable );

		//composite pk is not an identity.
		let members = _ds->ServerMeta().LoadTable( "main", "access_role_members" );
		let roleId = members->FindColumn( "role_id" );
		ASSERT_TRUE( roleId );
		EXPECT_FALSE( roleId->IsSequence );

		EXPECT_THROW( _ds->ServerMeta().LoadTable("main", "missing"), Exception );
	}

	TEST_P( SchemaTests, ServerMetaLoadTables ){
		let tables = _ds->ServerMeta().LoadTables( "main", "access_identities" ); //prefix filter, like 'access_identities%'.
		EXPECT_EQ( tables.size(), 1u );
		EXPECT_TRUE( tables.contains("access_identities") );
		EXPECT_FALSE( tables.contains("access_roles") ); //other access_ tables excluded by the prefix.
	}

	TEST_P( SchemaTests, ServerMetaIndexes ){
		let indexes = _ds->ServerMeta().LoadIndexes( "access_", {} );
		let named = find_if( indexes, [](let& i){ return i.Name=="access_identities_nk0"; } );
		ASSERT_NE( named, indexes.end() );
		EXPECT_TRUE( named->Unique );
		EXPECT_FALSE( named->PrimaryKey );
		EXPECT_EQ( named->Columns, (vector<string>{"name", "provider_id"}) );

		//rowid-alias single-integer pk is omitted by pragma_index_list; a composite pk surfaces as an autoindex, origin 'pk'.
		let pk = find_if( indexes, [](let& i){ return i.TableName=="access_role_members" && i.PrimaryKey; } );
		ASSERT_NE( pk, indexes.end() );
		EXPECT_EQ( pk->Columns, (vector<string>{"role_id", "member_id"}) );
	}

	TEST_P( SchemaTests, ServerMetaForeignKeys ){
		let fks = _ds->ServerMeta().LoadForeignKeys( "main" );
		let fk = find_if( fks, [](let& kv){ return kv.second.Table=="access_identities"; } );
		ASSERT_NE( fk, fks.end() );
		EXPECT_EQ( fk->second.pkTable, "access_providers" );
		EXPECT_EQ( fk->second.Columns, vector<string>{"provider_id"} );
	}

	TEST_P( SchemaTests, SyntaxDialect ){
		let& syntax = SqliteSyntax::Instance();
		EXPECT_EQ( syntax.ToString(EType::UInt), "integer" ); //rowid alias requires 'integer' exactly.
		EXPECT_EQ( syntax.ToString(EType::Long), "integer" );
		EXPECT_EQ( syntax.ToString(EType::VarChar), "varchar" );
		EXPECT_FALSE( syntax.HasProcs() );
		EXPECT_FALSE( syntax.CanAddForeignKeys() );
		EXPECT_EQ( syntax.GuidType(), "blob" );

		//Limit(): OFFSET requires a LIMIT, so skip-only must use a negative (=unbounded) limit, not 'limit 0' which returns zero rows.
		let sql = string{"select * from t"};
		EXPECT_EQ( syntax.Limit(sql, 10, 0), "select * from t limit 10" ); //limit-only.
		EXPECT_EQ( syntax.Limit(sql, 10, 5), "select * from t limit 10 offset 5" ); //limit + skip.
		EXPECT_EQ( syntax.Limit(sql, 0, 5), "select * from t limit -1 offset 5" ); //skip-only: returns the tail, not zero rows.

		//base MySQL dialect: every unsigned width must carry 'unsigned' (sqlite maps them all to 'integer', so it can't exercise this).
		let& my = MySqlSyntax::Instance();
		EXPECT_EQ( my.ToString(EType::UInt), "int unsigned" );
		EXPECT_EQ( my.ToString(EType::ULong), "bigint(20) unsigned" );
		EXPECT_EQ( my.ToString(EType::UInt16), "smallint unsigned" ); //#6: was "smallint" - uint16 columns got half the range.
		EXPECT_EQ( my.ToString(EType::UInt8), "tinyint unsigned" );
		EXPECT_EQ( my.ToString(EType::Int16), "smallint" ); //signed stays plain.
	}

	TEST_P( SchemaTests, AtSchemaMainOnly ){
		EXPECT_EQ( _ds->AtSchema("main").get(), _ds.get() );
		EXPECT_THROW( _ds->AtSchema("other"), Exception );
	}

	//#7: a single-table FromClause leaves Joins[0].To null; Contains/GetColumnPtr must not deref it.
	TEST_P( SchemaTests, SingleTableFromClauseNullJoin ){
		auto schema = DB::GetCluster( GetParam(), ms<Access::Authorize>("SqliteTests") )->GetAppSchema( "access" );
		auto table = schema->GetTablePtr( "identities" ); //config-side, Initialized -> Column::Table set, pk resolvable.
		ASSERT_TRUE( table );

		FromClause fc{ table }; //Joins[0].To is null (no join).
		ASSERT_FALSE( fc.HasJoin() );
		EXPECT_FALSE( fc.Contains("no_such_table") ); //was: deref of null To->Table.
		EXPECT_TRUE( fc.GetColumnPtr("id", SRCE_CUR) ); //surrogate pk, found in the From table.
		EXPECT_THROW( fc.GetColumnPtr("no_such_column", SRCE_CUR), Exception ); //typo'd column now THROWs instead of segfaulting on null To.
	}

	//#24: empty IN/NOT IN must not emit invalid `col in()` (debug ASSERT / release garbage) - use the 1=0 / 1=1 idiom.
	TEST_P( SchemaTests, EmptyInOperator ){
		auto schema = DB::GetCluster( GetParam(), ms<Access::Authorize>("SqliteTests") )->GetAppSchema( "access" );
		auto col = schema->GetTablePtr("identities")->GetColumnPtr( "id", SRCE_CUR );
		let& syntax = SqliteSyntax::Instance();
		EXPECT_EQ( syntax.FormatOperator(*col, EOperator::In, 0), "1=0" );    //empty IN matches nothing.
		EXPECT_EQ( syntax.FormatOperator(*col, EOperator::NotIn, 0), "1=1" ); //empty NOT IN matches everything.
		EXPECT_NE( syntax.FormatOperator(*col, EOperator::In, 2).find("in(?,?)"), string::npos ); //non-empty still an IN list.
	}

	//#40: SelectSql with empty columns overwrote the trailing space of "select " with '\n' ("select\nfrom..."). Guard the comma->newline replacement.
	TEST_P( SchemaTests, SelectSqlEmptyColumns ){
		auto schema = DB::GetCluster( GetParam(), ms<Access::Authorize>("SqliteTests") )->GetAppSchema( "access" );
		auto table = schema->GetTablePtr( "identities" );
		ASSERT_TRUE( table );
		let empty = DB::SelectSql( vector<sp<Column>>{}, FromClause{table}, WhereClause{} );
		EXPECT_TRUE( empty.Text.starts_with("select \n") ) << empty.Text; //space preserved, not clobbered to '\n'.
		EXPECT_FALSE( empty.Text.starts_with("select\n") ) << empty.Text; //the bug.
		let sks = DB::SelectSKsSql( table ); //non-empty columns still well-formed: trailing ',' -> newline, no dangling comma.
		EXPECT_TRUE( sks.Text.starts_with("select ") ) << sks.Text;
		EXPECT_EQ( sks.Text.find(",\n"), string::npos ) << sks.Text; //no comma left immediately before the from-newline.
	}

	//#15: View::Initialize must be idempotent - SchemaDdl::SyncTables re-initializes every table after the ctor's Initialize, and re-init duplicated PKTable->Children.
	TEST_P( SchemaTests, InitializeIdempotentChildren ){
		auto schema = DB::GetCluster( GetParam(), ms<Access::Authorize>("SqliteTests") )->GetAppSchema( "access" );
		sp<Table> mapped;
		for( let& kv : schema->Tables )
			if( kv.second->Map.has_value() && kv.second->Map->Parent->PKTable ){ mapped = kv.second; break; } //a parent/child-mapped table (e.g. members).
		ASSERT_TRUE( mapped );
		auto pkTable = mapped->Map->Parent->PKTable;
		let before = pkTable->Children.size();
		mapped->Initialize( schema, mapped ); //re-init, exactly as SyncTables does.
		EXPECT_EQ( pkTable->Children.size(), before ); //no duplicate child on re-init.
	}

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

	//#22: Value::operator=(uint) must return Value& (was `auto` -> deduced Value, i.e. a by-value copy).
	TEST( ValueTests, AssignUIntReturnsRef ){
		static_assert( std::is_same_v<decltype(std::declval<Value&>() = uint{5}), Value&>, "operator=(uint) must return Value&, not a copy." );
		Value v; v = uint{7};
		EXPECT_EQ( v.ToUInt(), 7u );
	}

	//#21: Value::ToUInt on a negative Double must not be UB - go through signed _int, matching the integer cases' modular wrap.
	TEST( ValueTests, ToUIntNegativeDouble ){
		EXPECT_EQ( Value{-5.0}.ToUInt(), (uint)(_int)-5 ); //modular wrap, deterministic (was UB).
		EXPECT_EQ( Value{-5.0}.ToInt(), -5 );              //ToInt = (_int)ToUInt round-trips.
		EXPECT_EQ( Value{42.0}.ToUInt(), 42u );            //non-negative unchanged.
	}

	//#12: DBException::Log must honor the base _logged protocol - a moved-from / already-logged exception must not re-log
	//(before the fix, the moved-from source logged a junk "sqle: " with its emptied Sql, and a BreakLog'd exception double-logged).
	TEST( DBExceptionTests, LogsOnce ){
		if( !Logging::FindLogger<Logging::MemoryLog>() )
			Logging::AddLogger( mu<Logging::MemoryLog>() ); //captures every level; self-contained, no shared-config change.
		let countSqle = []{ return Logging::Find( [](const Logging::Entry& e){ return e.Message().starts_with("sqle:"); } ).size(); };

		Logging::ClearMemory();
		{
			DB::Sql sql; sql.Text = "select dbexception_logonce";
			DBException src{ 7, move(sql), "boom", SRCE_CUR };
			DBException dst{ move(src) }; //src is now moved-from: Sql emptied, _logged=true.
		} //dtors run: dst logs exactly once; src stays silent (was the junk "sqle: " / double-log before the fix).
		EXPECT_EQ( countSqle(), 1u );
	}
}

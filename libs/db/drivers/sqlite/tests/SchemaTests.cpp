#include <gtest/gtest.h>
#include <jde/db/db.h>
#include <jde/db/meta/Column.h>
#include "../../../src/meta/IServerMeta.h"
#include "../../../src/meta/ddl/TableDdl.h" //complete type for LoadTable's sp<TableDdl>.
#include "../../../src/meta/ddl/Index.h" //complete type for LoadIndexes' vector<Index>.
#include "../../../src/meta/ddl/ForeignKey.h" //complete type for LoadForeignKeys' flat_map<string,ForeignKey>.
#include "../src/SqliteSyntax.h"
#include <jde/app/AppQL.h>

#define let const auto

namespace Jde::DB::Sqlite::Tests{
	struct SchemaTests : ::testing::TestWithParam<string>{
		α SetUp()ε->void override{
			let& cluster = GetParam();
			if( auto it=_dsByCluster.find(cluster); it!=_dsByCluster.end() ){ //sync each backend once.
				_ds = it->second;
				return;
			}
			Schema::Create( cluster );
			_ds = DS( cluster );
			_dsByCluster[cluster] = _ds;
		}
		static flat_map<string,sp<IDataSource>> _dsByCluster;
		sp<IDataSource> _ds;
	};
	flat_map<string,sp<IDataSource>> SchemaTests::_dsByCluster;
	INSTANTIATE_TEST_SUITE_P( Backends, SchemaTests,
		::testing::Values( "memory", "file" ),
		[]( let& info ){ return info.param; }
	);

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
	}

	TEST_P( SchemaTests, AtSchemaMainOnly ){
		EXPECT_EQ( _ds->AtSchema("main").get(), _ds.get() );
		EXPECT_THROW( _ds->AtSchema("other"), Exception );
	}
}

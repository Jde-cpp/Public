#include <jde/db/Row.h>
#include <jde/db/IDataSource.h>
#include <jde/db/sqlite_api.h> //RegisterAppServerProcs - apps/AppServer/config/sql/sqlite twins compiled in.
#include "../src/SqliteDataSource.h"
#include "../src/SqliteProcs.h"
#include "../src/SqliteSyntax.h"
#include "../../../src/meta/IServerMeta.h"
#include "../../../src/meta/ddl/ColumnDdl.h"
#include "../../../src/meta/ddl/ForeignKey.h"
#include "../../../src/meta/ddl/Index.h"
#include "../../../src/meta/ddl/Procedure.h"
#include "../../../src/meta/ddl/TableDdl.h"

#define let const auto

namespace Jde::DB::Sqlite{
	struct SqliteTests : public ::testing::Test{
		Ω SetUpTestSuite()->void{
			if( _ds )
				return;
			_ds = sp<IDataSource>{ GetDataSource() };
			jobject config;
			config["path"] = ":memory:";
			_ds->SetConfig( config );
			_ds->ExecuteSync( {"create table users( id integer not null, name varchar(255) not null, is_deleted bit not null default 0, weight float null, config text null, attributes varbinary null, created datetime not null default (unixepoch()), primary key(id) )"} );
			_ds->ExecuteSync( {"create table user_groups( user_id integer not null references users(id), group_id integer not null, primary key(user_id, group_id) )"} );
			_ds->ExecuteSync( {"create unique index users_name_ix on users( name )"} );
		}
		static sp<IDataSource> _ds;
	};
	sp<IDataSource> SqliteTests::_ds;

	TEST_F( SqliteTests, InsertSelectRoundTrip ){
		let now = DBTimePoint{ std::chrono::floor<std::chrono::seconds>(DBClock::now()) };
		let bytes = vector<uint8_t>{ 0xde, 0xad, 0xbe, 0xef };
		_ds->ExecuteSync( {"insert into users( name, is_deleted, weight, config, attributes, created ) values( ?, ?, ?, ?, ?, ? )",
			{Value{"alice"}, Value{true}, Value{1.5}, Value{}, Value{bytes}, Value{now}}} );

		let rows = _ds->Select( {"select name, is_deleted, weight, config, attributes, created from users where name=?", {Value{"alice"}}} );
		ASSERT_EQ( rows.size(), 1u );
		let& r = rows[0];
		EXPECT_EQ( r.GetString(0), "alice" );
		EXPECT_TRUE( r.GetBit(1) ); //declared 'bit' comes back as Bool, not int.
		EXPECT_DOUBLE_EQ( r.GetDouble(2), 1.5 );
		EXPECT_TRUE( r.IsNull(3) );
		EXPECT_EQ( r.GetBytes(4), bytes );
		EXPECT_EQ( r.GetTimePoint(5), now ); //declared 'datetime' comes back as Time from the stored epoch int.
	}

	TEST_F( SqliteTests, IdentityAndReturning ){
		let id1 = _ds->ExecuteScalerSync( {"insert into users( name ) values( ? ) returning id", {Value{"bob"}}}, EValue::UInt64 ).get_number<uint>();
		let id2 = _ds->ExecuteScalerSync( {"insert into users( name ) values( ? ) returning id", {Value{"carol"}}}, EValue::UInt64 ).get_number<uint>();
		EXPECT_EQ( id2, id1+1 ); //rowid alias auto-assigns - id omitted from the insert.
		let last = _ds->ExecuteScalerSync( {"select last_insert_rowid()"}, EValue::UInt64 ).get_number<uint>();
		EXPECT_EQ( last, id2 );
	}

	TEST_F( SqliteTests, DefaultNowApplied ){
		_ds->ExecuteSync( {"insert into users( name ) values( ? )", {Value{"dave"}}} );
		let rows = _ds->Select( {"select created from users where name=?", {Value{"dave"}}} );
		ASSERT_EQ( rows.size(), 1u );
		let created = rows[0].GetTimePoint( 0 ); //default (unixepoch()) - SqliteSyntax::NowDefault.
		let diff = std::chrono::abs( DBClock::now()-created );
		EXPECT_LT( diff, std::chrono::minutes{1} );
	}

	TEST_F( SqliteTests, NativeProcDispatch ){
		_ds->ExecuteSync( {"create table app_hosts( host_id integer not null, name varchar(255) not null, primary key(host_id) )"} );
		_ds->ExecuteSync( {"create table app_instances( instance_id integer not null, program_id int not null, name varchar(255) not null, host_id int not null references app_hosts(host_id), primary key(instance_id) )"} );
		RegisterAppServerProcs();

		//callers are unaware there's no server proc - same Sql shape InsertClause::Proc generates, trailing placeholder is the out param.
		Sql call1{ "app_instance_insert( ?, ?, ?, ? )", {Value{(uint)1}, Value{"gateway"}, Value{"host1"}, Value{}}, true };
		let instance1 = _ds->ExecuteScalerSync( move(call1), EValue::UInt64 ).get_number<uint>();
		Sql call2{ "app_instance_insert( ?, ?, ?, ? )", {Value{(uint)1}, Value{"gateway2"}, Value{"host1"}, Value{}}, true };
		let instance2 = _ds->ExecuteScalerSync( move(call2), EValue::UInt64 ).get_number<uint>();

		EXPECT_EQ( instance2, instance1+1 );
		let hosts = _ds->ExecuteScalerSync( {"select count(*) from app_hosts"}, EValue::UInt64 ).get_number<uint>();
		EXPECT_EQ( hosts, 1u ); //2nd call found the existing host inside the proc.

		Sql unregistered{ "no_such_proc( ? )", {Value{(uint)1}}, true };
		EXPECT_THROW( _ds->ExecuteSync(move(unregistered)), Exception );
	}

	TEST_F( SqliteTests, ServerMetaLoadTable ){
		let table = _ds->ServerMeta().LoadTable( "main", "users" );
		ASSERT_TRUE( table );
		ASSERT_EQ( table->Columns.size(), 7u );

		let id = table->FindColumn( "id" );
		ASSERT_TRUE( id );
		EXPECT_TRUE( id->IsSequence ); //single-column integer pk = rowid alias.
		EXPECT_FALSE( id->IsNullable );

		let name = table->FindColumn( "name" );
		ASSERT_TRUE( name );
		EXPECT_EQ( name->Type, EType::VarChar );
		EXPECT_EQ( name->MaxLength.value_or(0), 255u );

		let created = table->FindColumn( "created" );
		ASSERT_TRUE( created );
		EXPECT_EQ( created->Type, EType::DateTime );

		let weight = table->FindColumn( "weight" );
		ASSERT_TRUE( weight );
		EXPECT_TRUE( weight->IsNullable );

		//composite pk is not an identity.
		let groups = _ds->ServerMeta().LoadTable( "main", "user_groups" );
		let userId = groups->FindColumn( "user_id" );
		ASSERT_TRUE( userId );
		EXPECT_FALSE( userId->IsSequence );

		EXPECT_THROW( _ds->ServerMeta().LoadTable("main", "missing"), Exception );
	}

	TEST_F( SqliteTests, ServerMetaLoadTables ){
		let tables = _ds->ServerMeta().LoadTables( "main", "user" );
		EXPECT_EQ( tables.size(), 2u ); //users + user_groups, prefix filtered.
		EXPECT_TRUE( tables.contains("users") );
		EXPECT_TRUE( tables.contains("user_groups") );
	}

	TEST_F( SqliteTests, ServerMetaIndexes ){
		let indexes = _ds->ServerMeta().LoadIndexes( "user", {} );
		let named = find_if( indexes, [](let& i){ return i.Name=="users_name_ix"; } );
		ASSERT_NE( named, indexes.end() );
		EXPECT_TRUE( named->Unique );
		EXPECT_FALSE( named->PrimaryKey );
		EXPECT_EQ( named->Columns, vector<string>{"name"} );

		let pk = find_if( indexes, [](let& i){ return i.TableName=="user_groups" && i.PrimaryKey; } ); //composite pk -> autoindex, origin 'pk'.
		ASSERT_NE( pk, indexes.end() );
		EXPECT_EQ( pk->Columns, (vector<string>{"user_id", "group_id"}) );
	}

	TEST_F( SqliteTests, ServerMetaForeignKeys ){
		let fks = _ds->ServerMeta().LoadForeignKeys( "main" );
		let fk = find_if( fks, [](let& kv){ return kv.second.Table=="user_groups"; } );
		ASSERT_NE( fk, fks.end() );
		EXPECT_EQ( fk->second.pkTable, "users" );
		EXPECT_EQ( fk->second.Columns, vector<string>{"user_id"} );
	}

	TEST_F( SqliteTests, ServerMetaProcs ){
		RegisterProc( "meta_listed_proc", []( sqlite3&, const vector<Value>&, RowΛ*, SL )->uint{ return 0; } );
		let procs = _ds->ServerMeta().LoadProcs( "main" );
		EXPECT_TRUE( procs.contains("meta_listed_proc") ); //DDL sync treats registered procs as existing.
	}

	TEST_F( SqliteTests, SyntaxDialect ){
		let& syntax = SqliteSyntax::Instance();
		EXPECT_EQ( syntax.ToString(EType::UInt), "integer" ); //rowid alias requires 'integer' exactly.
		EXPECT_EQ( syntax.ToString(EType::Long), "integer" );
		EXPECT_EQ( syntax.ToString(EType::VarChar), "varchar" );
		EXPECT_FALSE( syntax.HasProcs() );
		EXPECT_FALSE( syntax.CanAddForeignKeys() );
		EXPECT_EQ( syntax.GuidType(), "blob" );
	}

	TEST_F( SqliteTests, AtSchemaMainOnly ){
		EXPECT_EQ( _ds->AtSchema("main").get(), _ds.get() );
		EXPECT_THROW( _ds->AtSchema("other"), Exception );
	}

	TEST_F( SqliteTests, RollbackOnProcThrow ){
		RegisterProc( "throwing_proc", []( sqlite3& db, const vector<Value>& params, RowΛ*, SL sl )->uint{
			ExecuteStatement( db, "insert into users( name ) values( ? )", {params[0]}, nullptr, sl );
			THROW( "proc failed after insert." );
		});
		let before = _ds->ExecuteScalerSync( {"select count(*) from users"}, EValue::UInt64 ).get_number<uint>();
		Sql call{ "throwing_proc( ? )", {Value{"rollback_user"}}, true };
		EXPECT_THROW( _ds->ExecuteSync(move(call)), Exception );
		let after = _ds->ExecuteScalerSync( {"select count(*) from users"}, EValue::UInt64 ).get_number<uint>();
		EXPECT_EQ( before, after ); //transaction rolled back.
	}
}

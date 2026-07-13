#include <jde/db/Row.h>
#include <jde/db/IDataSource.h>

#define let const auto

namespace Jde::DB::Sqlite::Tests{
	struct OpTests : ::testing::TestWithParam<string>{
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
	flat_map<string,sp<IDataSource>> OpTests::_dsByCluster;
	INSTANTIATE_TEST_SUITE_P( Backends, OpTests,
		::testing::Values( "memory", "file" ),
		[]( let& info ){ return info.param; }
	);
		//TODO Test Data, Test Procs, Test FKs, Test Indexes, Test Triggers.

	TEST_P( OpTests, InsertSelectRoundTrip ){
		let now = DBTimePoint{ std::chrono::floor<std::chrono::seconds>(DBClock::now()) };
		//provider_id left null - it has an fk to access_providers, and no provider row is seeded here.
		_ds->ExecuteSync( {"insert into access_identities( name, target, is_group, attributes, description, created ) values( ?, ?, ?, ?, ?, ? )",
			{Value{"alice"}, Value{"alice@example.com"}, Value{true}, Value{(uint)42}, Value{}, Value{now}}} );

		let rows = _ds->Select( {"select name, target, is_group, attributes, description, created from access_identities where name=?", {Value{"alice"}}} );
		ASSERT_EQ( rows.size(), 1u );
		let& r = rows[0];
		EXPECT_EQ( r.GetString(0), "alice" );
		EXPECT_EQ( r.GetString(1), "alice@example.com" );
		EXPECT_TRUE( r.GetBit(2) ); //declared 'bit' comes back as Bool, not int.
		EXPECT_EQ( r.GetUInt(3), 42u );
		EXPECT_TRUE( r.IsNull(4) ); //description not supplied.
		EXPECT_EQ( r.GetTimePoint(5), now ); //declared 'datetime' comes back as Time from the stored epoch int.
	}

	TEST_P( OpTests, IdentityAndReturning ){
		let id1 = _ds->ExecuteScalerSync( {"insert into access_identities( name, target ) values( ?, ? ) returning identity_id", {Value{"bob"}, Value{"bob@example.com"}}}, EValue::UInt64 ).get_number<uint>();
		let id2 = _ds->ExecuteScalerSync( {"insert into access_identities( name, target ) values( ?, ? ) returning identity_id", {Value{"carol"}, Value{"carol@example.com"}}}, EValue::UInt64 ).get_number<uint>();
		EXPECT_EQ( id2, id1+1 ); //rowid alias auto-assigns - identity_id omitted from the insert.
		let last = _ds->ExecuteScalerSync( {"select last_insert_rowid()"}, EValue::UInt64 ).get_number<uint>();
		EXPECT_EQ( last, id2 );
	}

	TEST_P( OpTests, DefaultNowApplied ){
		_ds->ExecuteSync( {"insert into access_identities( name, target ) values( ?, ? )", {Value{"dave"}, Value{"dave@example.com"}}} );
		let rows = _ds->Select( {"select created from access_identities where name=?", {Value{"dave"}}} );
		ASSERT_EQ( rows.size(), 1u );
		let created = rows[0].GetTimePoint( 0 ); //default (unixepoch()) - SqliteSyntax::NowDefault.
		let diff = std::chrono::abs( DBClock::now()-created );
		EXPECT_LT( diff, std::chrono::minutes{1} );
	}

	TEST_P( OpTests, NativeProcDispatch ){
		//access_permissions/access_roles created by Schema::Create. access_role_insert's twin inserts a permission row then the role,
		//returning role_id as the out param - callers are unaware there's no server proc (same Sql shape InsertClause::Proc generates).
		Sql call1{ "access_role_insert( ?, ?, ?, ?, ? )", {Value{"admin"}, Value{"admin"}, Value{}, Value{}, Value{}}, true };
		let role1 = _ds->ExecuteScalerSync( move(call1), EValue::UInt64 ).get_number<uint>();
		Sql call2{ "access_role_insert( ?, ?, ?, ?, ? )", {Value{"user"}, Value{"user"}, Value{}, Value{}, Value{}}, true };
		let role2 = _ds->ExecuteScalerSync( move(call2), EValue::UInt64 ).get_number<uint>();

		EXPECT_EQ( role2, role1+1 ); //permission_id rowid alias auto-assigns, reused as role_id.
		let roles = _ds->ExecuteScalerSync( {"select count(*) from access_roles"}, EValue::UInt64 ).get_number<uint>();
		EXPECT_EQ( roles, 2u );

		Sql unregistered{ "no_such_proc( ? )", {Value{(uint)1}}, true };
		EXPECT_THROW( _ds->ExecuteSync(move(unregistered)), Exception );
	}
}

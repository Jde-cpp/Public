#include <jde/db/Row.h>
#include <jde/db/DBException.h>
#include <jde/db/IDataSource.h>
#include "jde/fwk/settings.h"
#include <jde/access/Authorize.h>

#define let const auto

namespace Jde::DB::Sqlite::Tests{
	struct OpTests : BackendTests{};
	INSTANTIATE_BACKENDS( OpTests );
		//TODO Test Data, Test Procs, Test FKs, Test Indexes, Test Triggers.

	//#12: sqlite3_open_v2 allocates a handle even when it fails, so a failed open used to leave _db set - every later
	//call skipped the `if( !_db )` reopen and reused the dead handle, wedging the data source until process restart
	//even once the cause was gone. Not backend-parameterized: it owns a data source pointed at an unopenable path.
	TEST( ConnectionTests, ReopensAfterFailedOpen ){
		auto ds = DS( "wedge" ); //configured at a path whose directory does not exist.
		EXPECT_ANY_THROW( ds->ExecuteSync(Sql{"select 1"}, SRCE_CUR) ) << "open must fail";

		//cause removed: the next call has to open afresh. Before the fix _db stayed set, so this reused the dead handle and threw.
		ds->SetConfig( jobject{ {"catalogs", jobject{ {"testDb", jobject{ {"path", ":memory:"}, {"schemas", jobject{}}}} }} } );
		EXPECT_NO_THROW( ds->ExecuteSync(Sql{"select 1"}, SRCE_CUR) );
	}

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

	TEST_P( OpTests, InsertSeqSyncThroughProcTwin ){
		//InsertClause built from a proc name dispatches to a twin, so Execute returns ExecuteProc's rows-affected and
		//never reaches its last_insert_rowid line - the twin's out row is the only source of the new pk.  Pre-fix both
		//calls returned 1 (sqlite3_changes) and every caller silently shared one pk.
		//params: [0]=name, [1]=provider_id, [2]=target, [3]=attributes, [4]=description, [5]=is_group, [6]=email.
		let insert = []( string name, string target ){
			return DB::InsertClause{ "access_identity_insert",
				vector<Value>{Value{move(name)}, Value{}, Value{move(target)}, Value{}, Value{}, Value{false}, Value{}} };
		};
		let id1 = _ds->InsertSeqSync<uint>( insert("erin", "erin@example.com") );
		let id2 = _ds->InsertSeqSync<uint>( insert("frank", "frank@example.com") );
		EXPECT_GT( id1, 0u );
		EXPECT_EQ( id2, id1+1 ); //not 1,1 - identity_id is a rowid alias that auto-assigns.

		let rows = _ds->Select( {"select name from access_identities where identity_id=?", {Value{id2}}} );
		ASSERT_EQ( rows.size(), 1u ); //the returned pk must address the row just written.
		EXPECT_EQ( rows[0].GetString(0), "frank" );
	}

	TEST_P( OpTests, ProcArityGuard ){
		//Twins index params[N] positionally with unchecked operator[]; a short vector used to read past the end and
		//copy a Value variant from uninitialized memory.  RegisterProc's minParams turns that into a diagnosable throw.
		Sql tooFew{ "access_identity_insert( ?, ? )", {Value{"short"}, Value{}}, true }; //declares 7 params
		EXPECT_THROW( _ds->ExecuteSync(move(tooFew)), Exception );
		//...and the extra trailing out-param placeholder callers append must still be accepted (minParams is a floor).
		Sql extra{ "access_permission_insert( ?, ? )", {Value{false}, Value{(uint)0}}, true }; //declares 1
		EXPECT_NO_THROW( _ds->ExecuteSync(move(extra)) );
	}

	TEST_P( OpTests, ProcsSurviveSiblingTeardown ){
		{ //A 2nd data source over the same proc dlls - the registry is process-global, so its teardown must not strip procs _ds still dispatches.
			let sibling = DB::DataSource( Settings::AsObject(Ƒ("/dbServers/{}", GetParam())) );
		}
		Sql call{ "access_role_insert( ?, ?, ?, ?, ? )", {Value{"survivor"}, Value{"survivor"}, Value{}, Value{}, Value{}}, true };
		EXPECT_GT( _ds->ExecuteScalerSync(move(call), EValue::UInt64).get_number<uint>(), 0u );
	}

	TEST_P( OpTests, ConstraintErrorsMapped ){
		//Constraint violations used to surface as a bare Exception whose code was a crc of "step failed: {} - {}" - the
		//sqlite result code was dropped. They now carry the extended code and EDbError, so callers branch without knowing sqlite.
		_ds->ExecuteSync( {"insert into access_identities( name, target ) values( ?, ? )", {Value{"mapped1"}, Value{"mapped@example.com"}}} );
		try{
			_ds->ExecuteSync( {"insert into access_identities( name, target ) values( ?, ? )", {Value{"mapped2"}, Value{"mapped@example.com"}}} ); //access_identities_nk1 is unique.
			FAIL() << "duplicate target should have thrown.";
		}
		catch( const DBException& e ){
			EXPECT_EQ( ToString(e.Error), "duplicate" );
			EXPECT_EQ( e.Code(), 2067u ); //SQLITE_CONSTRAINT_UNIQUE - the extended code, not the bare SQLITE_CONSTRAINT(19).
		}
		try{ //fks are enforced (pragma foreign_keys=on) and no provider row is seeded.
			_ds->ExecuteSync( {"insert into access_identities( name, target, provider_id ) values( ?, ?, ? )", {Value{"mapped3"}, Value{"mappedFk@example.com"}, Value{(uint)999}}} );
			FAIL() << "unknown provider_id should have thrown.";
		}
		catch( const DBException& e ){
			EXPECT_EQ( ToString(e.Error), "foreignKey" );
		}
	}

	TEST_P( OpTests, DbSettingsHonoredAfterCache ){
		//The fixture already populated the global cluster cache - supplied dbSettings must still be honored, not silently swapped for the cache.
		auto auth = ms<Access::Authorize>( "SqliteTests" );
		EXPECT_THROW( DB::GetAppSchema("access", auth, jobject{{"scriptPaths", jarray{}}}), Exception ); //no cluster objects -> 'No db servers found.', not the cached schema.
		EXPECT_TRUE( DB::GetAppSchema("access", auth, Settings::AsObject("/dbServers")) );
	}
}

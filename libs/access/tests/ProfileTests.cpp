#include "gtest/gtest.h"
#include <jde/ql/ql.h>
#include <jde/ql/QLAwait.h>
#include <jde/ql/types/TableQL.h>
#include <jde/ql/types/MutationQL.h>
#include <jde/fwk/str.h>
#include <jde/db/IDataSource.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/IQL.h>
#include <jde/ql/LocalSubscriptions.h>
#include "globals.h"

#define let const auto
namespace Jde::Access::Tests{
	using namespace Json;
	class ProfileTests : public ::testing::Test{};
	Ω countRows( str table, sv column, uint id )ε->uint{ //by the table's json name:  SqlName carries the schema prefix and quotes a reserved word (the unprefixed `groups` on mysql).
		let& dbTable = *GetTable( table );
		return dbTable.Schema->DS()->ScalerSync<uint>( DB::Sql{Ƒ("select count(*) from {} where {}=?", dbTable.SqlName(), column), {DB::Value{id}}} );
	}

	Ω upsertProfile( sv target, sv value, UserPK executer )ε->jvalue{
		return QL().QuerySync<jvalue>( Ƒ("mutation updateProfile( \"target\":\"{}\", \"value\":{} )", target, value), {}, executer );
	}
	Ω deleteProfile( sv target, UserPK executer )ε->jvalue{
		return QL().QuerySync<jvalue>( Ƒ("mutation updateProfile( \"target\":\"{}\", \"value\":null )", target), {}, executer );
	}
	Ω selectProfile( sv target, UserPK executer )ε->jobject{
		return QL().QuerySync( Ƒ("profile( target:\"{}\" ){{ value }}", target), {}, executer );
	}

	TEST_F( ProfileTests, Crud ){
		let root = GetRoot();
		const UserPK userA{ GetId(GetUser("profileA", root)) };
		const UserPK userB{ GetId(GetUser("profileB", root)) };

		upsertProfile( "testKey", "\"v1\"", userA );//insert path
		ASSERT_EQ( AsSV(selectProfile("testKey", userA), "value"), "v1" );
		ASSERT_TRUE( selectProfile("testKey", userB).empty() );//scoped to the executer

		upsertProfile( "testKey", "\"{\\\"a\\\":1}\"", userA );//update path - realistic json blob with embedded quotes
		ASSERT_EQ( AsSV(selectProfile("testKey", userA), "value"), "{\"a\":1}" );

		upsertProfile( "testKey", "\"vB\"", userB );//same target, different user - separate rows
		ASSERT_EQ( AsSV(selectProfile("testKey", userB), "value"), "vB" );
		ASSERT_EQ( AsSV(selectProfile("testKey", userA), "value"), "{\"a\":1}" );

		deleteProfile( "testKey", userA );//value:null deletes
		ASSERT_TRUE( selectProfile("testKey", userA).empty() );
		ASSERT_EQ( AsSV(selectProfile("testKey", userB), "value"), "vB" );

		deleteProfile( "testKey", userB );
		PurgeUser( userA, root );
		PurgeUser( userB, root );
	}

	//#10: the save used to be update-then-insert-if-zero.  MySQL's row count is rows *changed*, so re-saving an identical
	//value answered 0, fell into the insert, and the (identity_id,target) pk turned an idempotent save into a 409 - while
	//sqlite and SQL Server, which count rows *matched*, succeeded.  A save is now one dialect upsert, so the second and
	//third writes below are no-ops rather than conflicts on every backend.  This passes on sqlite either way; it is the
	//MySQL path it exists to hold, which no ctest suite reaches.
	TEST_F( ProfileTests, RepeatedIdenticalSave ){
		let root = GetRoot();
		const UserPK user{ GetId(GetUser("profileRepeat", root)) };
		EXPECT_NO_THROW( upsertProfile("repeatKey", "\"same\"", user) );//insert
		EXPECT_NO_THROW( upsertProfile("repeatKey", "\"same\"", user) );//identical - the 409 used to land here
		EXPECT_NO_THROW( upsertProfile("repeatKey", "\"same\"", user) );
		EXPECT_EQ( AsSV(selectProfile("repeatKey", user), "value"), "same" );

		EXPECT_NO_THROW( upsertProfile("repeatKey", "\"changed\"", user) );//a real change still lands
		EXPECT_EQ( AsSV(selectProfile("repeatKey", user), "value"), "changed" );

		deleteProfile( "repeatKey", user );
		PurgeUser( user, root );
	}

	TEST_F( ProfileTests, Anonymous ){
		EXPECT_THROW( upsertProfile("anonKey", "\"v\"", UserPK{}), Exception );
		EXPECT_THROW( upsertProfile("anonKey", "\"v\"", UserPK{UserPK::System}), Exception );
	}

	//access-review3 #14:  nothing purged access_profiles, so purgeUser failed on the profiles fk for anyone who had used the UI -
	//Crud above deletes both profiles before it purges, which is exactly what masked it.  users has a purgeProc now that takes the
	//identity's children first.
	TEST_F( ProfileTests, PurgeUserWithProfile ){
		let root = GetRoot();
		const UserPK user{ GetId(GetUser("profilePurge", root)) };
		upsertProfile( "purgeKey", "\"v\"", user );
		ASSERT_EQ( countRows("profiles", "identity_id", user.Value), 1u );
		EXPECT_NO_THROW( PurgeUser(user, root) ); //used to throw on the fk.
		EXPECT_EQ( countRows("profiles", "identity_id", user.Value), 0u );
		EXPECT_EQ( countRows("identities", "identity_id", user.Value), 0u );
		EXPECT_TRUE( SelectUser("profilePurge", root, nullopt, true).empty() );
	}
	//and access_provider_purge, which deletes users and identities directly, takes the same children first.
	TEST_F( ProfileTests, PurgeProviderWithUsersInUse ){
		let root = GetRoot();
		constexpr sv providerTarget{ "purgeProviderTest" };
		auto provider = QL().QuerySync( Ƒ("provider( name:\"{}\" ){{ id }}", providerTarget), {}, root ); //providers_ql has no target column; the insert proc names it after the target.
		if( provider.empty() )
			provider = QL().QuerySync( Ƒ("createProvider( target:\"{}\", providerType:{} ){{ id }}", providerTarget, underlying(EProviderType::Key)), {}, root );
		const ProviderPK providerPK{ (ProviderPK)GetId(provider) };
		const UserPK user{ GetId(GetUser("purgeProviderUser", root, false, providerPK)) };
		upsertProfile( "purgeKey", "\"v\"", user );
		const GroupPK group{ GetId(GetGroup("purgeProviderGroup", root)) };
		AddToGroup( group, {user}, root );
		ASSERT_EQ( countRows("groups", "member_id", user.Value), 1u );
		EXPECT_NO_THROW( Purge("provider", providerPK, root) );
		EXPECT_EQ( countRows("profiles", "identity_id", user.Value), 0u );
		EXPECT_EQ( countRows("groups", "member_id", user.Value), 0u );
		EXPECT_EQ( countRows("identities", "identity_id", user.Value), 0u );
		EXPECT_EQ( countRows("providers", "provider_id", providerPK), 0u );
		PurgeGroup( group, root );
	}

	//access-review3 #16:  ProfileAwait scoped every statement to the executer and then handed the mutation to the fan-out, which
	//delivered its args - target and the whole value blob - to every profileUpdated subscriber, no per-listener identity, no
	//gate (profiles is ops:None, so subscribing is open to anyone logged in).  Profiles are per-user state:  nothing is broadcast.
	struct ProfileListener final : QL::IListener{
		ProfileListener()ι: QL::IListener{"ProfileTests"}{}
		α OnChange( const jvalue& j, QL::SubscriptionId )ε->void override{ Changes.push_back( Json::AsObject(j) ); }
		α OnTraces( App::Proto::FromServer::Traces&& )ι->void override{ ASSERT(false); }
		vector<jobject> Changes;
	};
	TEST_F( ProfileTests, WritesAreNotBroadcast ){
		let root = GetRoot();
		const UserPK userA{ GetId(GetUser("profileLeakA", root)) };
		const UserPK userB{ GetId(GetUser("profileLeakB", root)) };
		auto listener = ms<ProfileListener>();
		BlockTAwait<vector<QL::SubscriptionId>>( move(*QLPtr()->Subscribe("subscription ProfileUpdated{ profileUpdated(subscriptionId:$id){ target value } }", jobject{{"id",7716}}, listener, userB)) );
		upsertProfile( "secret", "\"userA-only\"", userA );
		EXPECT_EQ( AsSV(selectProfile("secret", userA), "value"), "userA-only" ); //the write itself still lands.
		QL::Subscriptions::StopListen( listener );
		EXPECT_TRUE( listener->Changes.empty() ) << "B received A's profile: " << serialize( listener->Changes[0] );
		deleteProfile( "secret", userA );
		PurgeUser( userA, root );
		PurgeUser( userB, root );
	}

	//access-review3 #9 (ql-review3 #5):  Server::CustomQuery scopes profiles to the executer with TableQL::AddFilter, which nested
	//the criterion inside a client-supplied `filter` arg through as_object() - noexcept, so `filter:1` from an anonymous GET was a
	//terminate ahead of any authorization.  The junk arg is now refused as an unknown column, as an exception the caller gets.
	TEST_F( ProfileTests, ScalarFilterArgIsRefusedNotFatal ){
		let root = GetRoot();
		const UserPK user{ GetId(GetUser("profileFilter", root)) };
		EXPECT_THROW( QL().QuerySync<jvalue>("profile( filter:1, target:\"testKey\" ){ value }", {}, user), Exception );
		EXPECT_THROW( QL().QuerySync<jvalue>("profile( filter:1, target:\"testKey\" ){ value }", {}, UserPK{}), Exception ); //the unauthenticated shape.
		PurgeUser( user, root );
	}
}

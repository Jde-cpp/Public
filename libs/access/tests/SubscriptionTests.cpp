//The subscription fan-out (QL::Subscriptions::OnMutation) recovers the mutated row's id when the mutation result didn't carry
//one.  Review #7: every failure in that lookup escaped to the fan-out's catch _before_ the first OnChange, so one awkward arg
//cost *every* subscriber the notification.  These tests pin the reachable shapes.
//Deletes are the no-id path here: UpdateAwait resumes with a bare rowCount, so `available` is the args alone (an insert on a
//table with an insert proc - every access table - gets its id back from the proc's out row and never reaches the lookup).
//This suite is the schema-backed one closest to QL (sqlite in-memory under ctest), so the coverage lives here rather than in
//Jde.QL.Tests, which opens no data source.
#include "gtest/gtest.h"
#include <jde/access/AccessListener.h>
#include <jde/access/Authorize.h>
#include <jde/access/awaits/EventsSubscribeAwait.h>
#include <jde/access/server/awaits/RoleAwait.h>
#include "../src/accessInternal.h"
#include "globals.h"
#include <jde/ql/IQL.h>
#include <jde/ql/LocalSubscriptions.h>

#define let const auto

namespace Jde::Access::Tests{
	constexpr sv Schema{ "qlSubTests" }; //not "access": ResourceTests::CheckDefaults counts that schema's resources.

	struct TestListener final : QL::IListener{
		TestListener()ι: QL::IListener{"SubscriptionTests"}{}
		α OnChange( const jvalue& j, QL::SubscriptionId )ε->void override{ Changes.push_back( Json::AsObject(j) ); }
		α OnTraces( App::Proto::FromServer::Traces&& )ι->void override{ ASSERT(false); }
		α Resource( uint i )Ι->const jobject&{ return Json::AsObject( Changes[i].begin()->value() ); } //{"resources":{…}} - the fields the subscription asked for.
		vector<jobject> Changes;
	};

	//mirrors Access::EventsSubscribeAwait's format - "subscription ResourcesDeleted{ resourcesDeleted(subscriptionId:$id){…} }".
	Ω listenTo( sv text )ε->sp<TestListener>{
		auto y = ms<TestListener>();
		auto await = QLPtr()->Subscribe( string{text}, jobject{{"id",7717}}, y, UserPK{UserPK::System} );
		BlockTAwait<vector<QL::SubscriptionId>>( move(*await) );
		return y;
	}
	Ω listen()ε->sp<TestListener>{ return listenTo( "subscription ResourcesDeleted{ resourcesDeleted(subscriptionId:$id){id target} }" ); }
	Ω createResource( sv target, sv extraArgs="" )ε->void{
		let ql = Ƒ( R"(mutation createResource( schemaName:"{0}", name:"{1} - name", target:"{1}", description:"{1} - description"{2} ))", Schema, target, extraArgs );
		QL().QuerySync<jvalue>( ql, {}, GetRoot() );
	}
	Ω deleteResource( sv target, sv extraArgs="" )ε->void{
		QL().QuerySync<jvalue>( Ƒ(R"(mutation deleteResource( target:"{}"{} ))", target, extraArgs), {}, GetRoot() );
	}
	Ω resourceIds( sv target )ε->vector<uint32>{
		let ql = Ƒ( R"(resources( schemaName:"{}", target:"{}" ){{ id target deleted }})", Schema, target );
		vector<uint32> y;
		for( let& v : QL().QuerySync<jarray>(ql, {}, GetRoot()) )
			y.push_back( Json::AsNumber<uint32>(Json::AsObject(v), "id") );
		return y;
	}

	//A null arg is a value the client may send ("deleted: null"), and `created` is server-defaulted (insertable:false,
	//default:now), so the old `created is null` term matched no row, ScalerSync threw on the empty result, and the fan-out died
	//before notifying anyone.  Nulls no longer join the lookup, so the other args still find the row.
	//It used to arrive here as an unbound `$missing`, which extrapolated to the same null;  #36 made that its own error, so the
	//shape findId actually has to survive is spelled literally.  It also used to be `created:null`, which CreateDeleteRestore ands
	//into the where clause - `created` is server-defaulted, so that deleted 0 rows and #47 now suppresses its notification.
	TEST( SubscriptionTests, NullArgDoesNotDropTheNotification ){
		constexpr sv target{ "subNullArg" };
		createResource( target );
		let ids = resourceIds( target ); ASSERT_EQ( ids.size(), 1u );

		auto listener = listen();
		deleteResource( target, ", criteria:null" );//criteria is nullable and this row has none, so the delete matches - #47: a
		                                              //statement that matches nothing no longer notifies, so the null has to be a real one.
		QL::Subscriptions::StopListen( listener );

		ASSERT_EQ( listener->Changes.size(), 1u );
		let& resource = listener->Resource( 0 );
		EXPECT_EQ( Json::AsSV(resource, "target"), target );
		EXPECT_EQ( Json::AsNumber<uint32>(resource, "id"), ids[0] ); //recovered from the remaining args, the null skipped.
	}

	//The ordinary shape: the args identify exactly one row.
	TEST( SubscriptionTests, IdRecoveredFromTheArgs ){
		constexpr sv target{ "subPlain" };
		createResource( target );
		let ids = resourceIds( target ); ASSERT_EQ( ids.size(), 1u );

		auto listener = listen();
		deleteResource( target );
		QL::Subscriptions::StopListen( listener );

		ASSERT_EQ( listener->Changes.size(), 1u );
		EXPECT_EQ( Json::AsNumber<uint32>(listener->Resource(0), "id"), ids[0] );
	}

	//#8: Access::EventsSubscribeAwait subscribed as "permission", which the parser keys ToPlural(FromJson("permission")) =
	//`permissions`, while the UI's updatePermissionRight publishes under `permission_rights` - OnMutation's exact-key lookup
	//missed every time, so AccessListener::PermissionUpdated never fired and a revoked grant stayed in force until restart.
	//Published straight into OnMutation rather than through a real update:  the subject is the key MutationQL derives, and #47
	//means a statement that matched nothing no longer publishes - so a mutation against an id that does not exist would prove
	//nothing here.  jvalue{1} is the row count a real one would carry.
	TEST( SubscriptionTests, PermissionRightUpdateNotifiesItsSubscriber ){
		auto listener = listenTo( "subscription PermissionRightUpdated{ permissionRightUpdated(subscriptionId:$id){ id allowed denied } }" ); //what access subscribes now.
		let m = QL::ParseM( "mutation updatePermissionRight( id:987654321, allowed:1, denied:0 )", {}, Schemas() );
		ASSERT_EQ( m.TableName(), "permission_rights" );//the spelling #8 was about.
		QL::Subscriptions::OnMutation( m, jvalue{1} );
		QL::Subscriptions::StopListen( listener );

		ASSERT_EQ( listener->Changes.size(), 1u );
		let& permission = listener->Resource( 0 );
		EXPECT_EQ( Json::AsNumber<uint>(permission, "id"), 987654321u );
		EXPECT_EQ( Json::AsNumber<uint>(permission, "allowed"), 1u );
	}
	//the old spelling, kept as the control:  it keys a different table, so the same mutation reaches nobody.
	TEST( SubscriptionTests, PermissionUpdateKeysADifferentTable ){
		auto listener = listenTo( "subscription PermissionUpdated{ permissionUpdated(subscriptionId:$id){ id allowed denied } }" );
		QL::Subscriptions::OnMutation( QL::ParseM("mutation updatePermissionRight( id:987654321, allowed:1, denied:0 )", {}, Schemas()), jvalue{1} );
		QL::Subscriptions::StopListen( listener );
		EXPECT_TRUE( listener->Changes.empty() ); //`permissions` != `permission_rights` - this is what #8 was.
	}

	//An ambiguous lookup used to broadcast whichever row the driver returned last; now the notification goes out without an id
	//(and logs a warning) rather than naming a row at random.  `criteria` is part of the natural key, so the two rows differ only there.
	TEST( SubscriptionTests, AmbiguousLookupNotifiesWithoutAnId ){
		constexpr sv target{ "subAmbiguous" };
		createResource( target, R"(, criteria:"a")" );
		createResource( target, R"(, criteria:"b")" );
		ASSERT_EQ( resourceIds(target).size(), 2u );

		auto listener = listen();
		deleteResource( target ); //by target: both rows match, so the id lookup can't pick one.
		QL::Subscriptions::StopListen( listener );

		ASSERT_EQ( listener->Changes.size(), 1u ); //the notification still went out.
		EXPECT_FALSE( listener->Resource(0).contains("id") );
		EXPECT_EQ( Json::AsSV(listener->Resource(0), "target"), target );
	}

	//ql-review3 #43: the resources subscriptions prefixed their schema predicate to the *column* list, where LoadTable turned it
	//into the columns '(', 'schema:$schemas' and ')' and it never applied.  It is a predicate on the subscription, so it belongs
	//in the subscription's argument list next to subscriptionId - which is where EventsSubscribeAwait's format now puts it.
	//This is that text, in the shape the await emits it;  the parse itself is the pin, since the guard added with #43 refuses
	//the old spelling outright (Access.Tests would not have started).
	TEST( SubscriptionTests, TheSchemaPredicateIsAnArgumentNotAColumn ){
		let text = R"(subscription ResourcesCreated{ resourcesCreated(subscriptionId:$id, schemaName:$schemas){ id schemaName target criteria deleted } })";
		auto subs = QL::ParseSubscriptions( string{text}, jobject{{"id",7717},{"schemas",jarray{Schema}}}, Schemas() );
		ASSERT_EQ( subs.size(), 1u );
		let& fields = subs[0].Fields;
		EXPECT_EQ( subs[0].TableName, "resources" );
		EXPECT_EQ( subs[0].Id, 7717u );//subscriptionId is consumed off the args; schemaName is not.
		ASSERT_TRUE( fields.Args.contains("schemaName") ) << serialize( fields.Args );
		let schemaArg = fields.FindPtr<jarray>( "schemaName" );//$schemas is the caller's list, so it extrapolates to an array.
		ASSERT_TRUE( schemaArg ) << serialize( fields.Args );
		ASSERT_EQ( schemaArg->size(), 1u );
		EXPECT_EQ( (*schemaArg)[0].as_string(), Schema );//resolved through Variables - a filter could read it.
		EXPECT_EQ( fields.Columns.size(), 5u );//the five real ones - no '(' or ')' among them.
		for( let& c : fields.Columns )
			EXPECT_EQ( c.JsonName.find_first_of("()"), string::npos ) << c.JsonName;
	}
	//and the shape it replaced is now refused rather than silently mis-parsed.
	TEST( SubscriptionTests, ThePredicateInTheColumnListIsRefused ){
		let text = R"(subscription ResourcesCreated{ resourcesCreated(subscriptionId:$id){ (schemaName:$schemas)id target } })";
		try{
			QL::ParseSubscriptions( string{text}, jobject{{"id",7717},{"schemas",jarray{Schema}}}, Schemas() );
			ADD_FAILURE() << "the misplaced predicate parsed";
		}
		catch( const Exception& e ){
			EXPECT_NE( string{e.what()}.find("argument list where a column belongs"), string::npos ) << e.what();
		}
	}

	//ql-review3 #47: OnMutation never looked at the result, and an integer rowCount is not an object - so `available` was the
	//args alone and the id the client sent went straight to the listeners.  A 0-row delete therefore told AccessListener a row
	//had been deleted: Authorize::DeleteUser marks it IsDeleted in memory and every later request by that user is refused with
	//"User is deleted", until restart, with the db row untouched.  CreateDeleteRestore ands every extra column arg into the
	//where clause, which is how a delete matches nothing while still looking like one.
	TEST( SubscriptionTests, AZeroRowDeleteDoesNotNotify ){
		constexpr sv target{ "subZeroRow" };
		createResource( target );
		let ids = resourceIds( target ); ASSERT_EQ( ids.size(), 1u );

		auto listener = listen();
		//`name`, not `schemaName`:  CreateDeleteRestore looks the extra arg up with Table::FindColumn, which takes the sql name, so
		//only args whose json and sql spellings coincide ever reach the where clause.  That is the finding's own `name:"nomatch"`.
		deleteResource( target, ", name:\"nomatch\"" );//matches no row - and is reported as success.
		QL::Subscriptions::StopListen( listener );

		EXPECT_TRUE( listener->Changes.empty() ) << "a statement that matched nothing was published as an event";
		EXPECT_EQ( resourceIds(target).size(), 1u ) << "the row really was untouched - that is the point";//soft-deleted rows drop out.
	}
	//the control, on the same row: the delete that does match still notifies.
	TEST( SubscriptionTests, TheSameDeleteWithoutTheExtraPredicateDoesNotify ){
		constexpr sv target{ "subZeroRowControl" };
		createResource( target );
		ASSERT_EQ( resourceIds(target).size(), 1u );

		auto listener = listen();
		deleteResource( target );
		QL::Subscriptions::StopListen( listener );
		EXPECT_EQ( listener->Changes.size(), 1u );
	}

	//ql-review3 #53: the generic fan-out never evaluated a subscription's own arguments - only SubscribeLog::Write did, column by
	//column - so a client that asked for one schema was delivered every schema's events.  End to end here because the values the
	//predicate is tested against are the mutation's args, which only a real mutation produces.
	TEST( SubscriptionTests, ASubscriptionsOwnArgsScopeIt ){
		constexpr sv target{ "subFiltered" };
		auto match = listenTo( Ƒ(R"(subscription ResourcesCreated{{ resourcesCreated(subscriptionId:$id, schemaName:"{}"){{ id target schemaName }} }})", Schema) );
		auto miss = listenTo( R"(subscription ResourcesCreated{ resourcesCreated(subscriptionId:$id, schemaName:"nomatch"){ id target schemaName } })" );

		createResource( target );
		QL::Subscriptions::StopListen( match );
		QL::Subscriptions::StopListen( miss );

		ASSERT_EQ( match->Changes.size(), 1u );
		EXPECT_EQ( Json::AsSV(match->Resource(0), "target"), target );
		EXPECT_TRUE( miss->Changes.empty() ) << "the subscriber asked for one schema and was given another's event";
	}

	//ql-review3 #55: the fan-out delivers without an id on purpose when the lookup was ambiguous or found nothing (ql-review2 #7),
	//and AccessListener::OnChange read that id with Json::AsNumber, which throws - into the fan-out's catch, which was empty.  So
	//the real listener in this binary skipped those events and its cache went stale with nothing in the log.  Since access-review3
	//#22 a resources event without an id is resolved by target instead (IdLessResourceDeleteReachesTheCache below); one naming a
	//target the cache does not hold is refused by name - the mirror of the unknown-pk case - and an event with neither is the
	//one that says the cache is stale and returns.  Called directly rather than through a mutation, because a throw here is
	//invisible from the outside - ql swallows it either way, which is the other half (SubscriptionsTests.AThrowingListenerIsWarnedAbout).
	TEST( SubscriptionTests, AnIdLessNotificationDoesNotThrowOutOfTheListener ){
		auto listener = ms<Access::AccessListener>( QLPtr() );
		let deleted = (QL::SubscriptionId)underlying( ESubscription::Resources|ESubscription::Deleted );
		jvalue nameless{ jobject{ {"resources", jobject{{"description","nothing to find it by"}}} } };
		EXPECT_NO_THROW( listener->OnChange(nameless, deleted) );
		jvalue idLess{ jobject{ {"resources", jobject{{"target","subStaleCache"}}} } };//exactly what AmbiguousLookupNotifiesWithoutAnId delivers.
		try{
			listener->OnChange( idLess, deleted );
			ADD_FAILURE() << "an id-less event naming a target the cache does not hold was accepted";
		}
		catch( const Exception& e ){
			EXPECT_NE( string{e.what()}.find("subStaleCache"), string::npos ) << e.what();//dispatched and refused by name, not skipped.
		}

		//and a payload that does carry one is still dispatched - the tolerance must not swallow the ordinary case.  An id nothing
		//is registered under reaches ResourceChanged and is refused *there*, by pk, which is the proof that it got that far.
		jvalue withId{ jobject{ {"resources", jobject{{"id",987654321},{"target","subStaleCache"}}} } };
		try{
			listener->OnChange( withId, deleted );
			ADD_FAILURE() << "an unknown resource pk was accepted";
		}
		catch( const Exception& e ){
			EXPECT_NE( string{e.what()}.find("987654321"), string::npos ) << e.what();//it was dispatched with the id, not skipped.
		}
	}

	//access-review3 #25:  AccessListener::Shutdown unsubscribed through UnsubscribeAwait with IListener::Ids, which nothing ever
	//filled - an empty set short-circuits it - so every subscription, and the listener with it, outlived the app's own handle.
	//It goes through StopListen by listener now, which takes everything when given no ids.  A second listener, subscribed the way
	//Configure subscribes the real one, so the startup listener the rest of this suite relies on is not the one shut down.
	TEST( SubscriptionTests, ShutdownUnsubscribesTheListener ){
		auto listener = ms<Access::AccessListener>( QLPtr() );
		BlockVoidAwait( Access::EventsSubscribeAwait{QLPtr(), {"access"}, UserPK{UserPK::System}, listener} );
		listener->Shutdown( false, SRCE_CUR );
		EXPECT_TRUE( QL::Subscriptions::StopListen(listener, {}).empty() ) << "Shutdown left subscriptions registered";
	}

	//access-review3 #22 (with #23's column):  tolerating the id-less event was half of it - the cache still kept enforcing rows the
	//admin had just deleted.  Resources recover by schemaName+target, for every row the by-target delete hit, through the startup
	//listener's own subscription - which asked for a column called `schema` that no table has, so the name could never match.
	//In `access`, where that subscription's schema predicate lets the events through; criteria-scoped, so CheckDefaults is untouched.
	TEST( SubscriptionTests, IdLessResourceDeleteReachesTheCache ){
		let root = GetRoot();
		constexpr sv target{ "subIdLessDelete" };
		let select = Ƒ( R"(resources( schemaName:"access", target:"{}" ){{ id }})", target );
		for( let& v : QL().QuerySync<jarray>(select, {}, root) ) //a previous run's rows.
			Purge( "resource", GetId(Json::AsObject(v)), root );
		for( sv criteria : {"a", "b"} )
			QL().QuerySync<jvalue>( Ƒ(R"(mutation createResource( schemaName:"access", name:"{0}", target:"{0}", description:"{0}", criteria:"{1}", allowed:255 ))", target, criteria), {}, root );
		vector<uint32> ids;
		for( let& v : QL().QuerySync<jarray>(select, {}, root) )
			ids.push_back( Json::AsNumber<uint32>(Json::AsObject(v), "id") );
		ASSERT_EQ( ids.size(), 2u );
		const UserPK nobody{ GetId(GetUser("subIdLessNobody", root)) };
		for( let id : ids )
			EXPECT_THROW( Authorizer()->TestAdmin((ResourcePK)id, nobody), Exception ) << "active in the cache, so enforced";

		deleteResource( target ); //by target:  both rows, and a notification with no id.
		for( let id : ids )
			EXPECT_NO_THROW( Authorizer()->TestAdmin((ResourcePK)id, nobody) ) << "deleted in the cache as well - a deleted resource fail-opens";

		for( let id : ids )
			Purge( "resource", id, root );
		PurgeUser( nobody, root );
	}

	//appserver-review3 #13:  the server's Configure subscribes with no schema predicate now - its Authorize gates and answers every
	//schema's grants (TestSchemaAdmin, and the flat rule behind TestAdmin), so it has to see every schema's rows change.  This
	//suite's schema is not "access":  before, the startup listener's resourcesCreated(schemaName:["access"]) dropped the event and
	//the row never reached the cache, so a nobody passed TestAdmin on it.
	TEST( SubscriptionTests, OtherSchemasResourcesReachTheStartupCache ){
		let root = GetRoot();
		constexpr sv target{ "subAllSchemas" };
		for( let id : resourceIds(target) )//a previous run's row.
			Purge( "resource", id, root );
		createResource( target, ", allowed:255" );
		let ids = resourceIds( target ); ASSERT_EQ( ids.size(), 1u );
		const UserPK nobody{ GetId(GetUser("subAllSchemasNobody", root)) };
		EXPECT_THROW( Authorizer()->TestAdmin((ResourcePK)ids[0], nobody), Exception ) << "created in the cache, so enforced";
		deleteResource( target );
		EXPECT_NO_THROW( Authorizer()->TestAdmin((ResourcePK)ids[0], nobody) ) << "deleted in the cache as well";
		Purge( "resource", ids[0], root );
		PurgeUser( nobody, root );
	}

	//...and role grants:  RoleMAwait::AddPermission delivers roleAdded only to subscribers whose resource(schemaName:…) names the
	//permission's schema - the server's listener names none now - so a grant on another schema reaches the cache:  a holder of the
	//role administers the resource, which it did not before the grant.  The row is created first (the previous pin) so the grant's
	//admin check has something to enforce - System grants, as root holds nothing on a new row.
	TEST( SubscriptionTests, OtherSchemasRoleGrantsReachTheStartupCache ){
		let root = GetRoot();
		constexpr sv target{ "subAllSchemasRole" };
		for( let id : resourceIds(target) )//a previous run's row.
			Purge( "resource", id, root );
		createResource( target, ", allowed:255" );
		let ids = resourceIds( target ); ASSERT_EQ( ids.size(), 1u );
		let resourcePK = (ResourcePK)ids[0];
		const RolePK rolePK{ (RolePK)GetId(Get("role", "subAllSchemasRole", root)) };
		const UserPK holder{ GetId(GetUser("subAllSchemasHolder", root)) };
		QL().QuerySync<jvalue>( Ƒ("mutation createAcl( identity:{{id:{}}}, role:{{id:{}}} )", holder.Value, rolePK), {}, root );
		EXPECT_THROW( Authorizer()->TestAdmin(resourcePK, holder), Exception ) << "no grant yet";
		let grant = Ƒ( R"(addRole( id:{}, permissionRight:{{ allowed:{}, denied:0, resource:{{ schemaName:"{}", target:"{}" }} }} ))", rolePK, underlying(ERights::Administer), Schema, target );
		let added = BlockTAwait<jvalue>( Server::RoleMAwait{QL::ParseM(grant, {}, Schemas()), UserPK{UserPK::System}} ).as_object();
		EXPECT_NO_THROW( Authorizer()->TestAdmin(resourcePK, holder) ) << "roleAdded did not reach the cache - the holder's role does not carry the grant";

		BlockTAwait<jvalue>( Server::RoleMAwait{QL::ParseM(Ƒ("mutation removeRole( id:{}, permissionRight:{{id:{}}} )", rolePK, Json::AsNumber<PermissionPK>(added, "permissionRight/id")), {}, Schemas()), UserPK{UserPK::System}} );
		QL().QuerySync<jvalue>( Ƒ("purgeAcl( identity:{{ id:{} }}, role:{{ id:{} }} )", holder.Value, rolePK), {}, root );
		Purge( "role", rolePK, root );
		Purge( "resource", resourcePK, root );
		PurgeUser( holder, root );
	}
}

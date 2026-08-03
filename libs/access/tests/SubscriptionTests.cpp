//The subscription fan-out (QL::Subscriptions::OnMutation) recovers the mutated row's id when the mutation result didn't carry
//one.  Review #7: every failure in that lookup escaped to the fan-out's catch _before_ the first OnChange, so one awkward arg
//cost *every* subscriber the notification.  These tests pin the reachable shapes.
//Deletes are the no-id path here: UpdateAwait resumes with a bare rowCount, so `available` is the args alone (an insert on a
//table with an insert proc - every access table - gets its id back from the proc's out row and never reaches the lookup).
//This suite is the schema-backed one closest to QL (sqlite in-memory under ctest), so the coverage lives here rather than in
//Jde.QL.Tests, which opens no data source.
#include "gtest/gtest.h"
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
	Ω listen()ε->sp<TestListener>{
		auto y = ms<TestListener>();
		auto await = QLPtr()->Subscribe( "subscription ResourcesDeleted{ resourcesDeleted(subscriptionId:$id){id target} }", jobject{{"id",7717}}, y, UserPK{UserPK::System} );
		BlockTAwait<vector<QL::SubscriptionId>>( move(*await) );
		return y;
	}
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

	//A $variable with no value extrapolates to json null (Input::ExtrapolateVariables).  `created` is server-defaulted
	//(insertable:false, default:now), so the old `created is null` term matched no row, ScalerSync threw on the empty result,
	//and the fan-out died before notifying anyone.  Nulls no longer join the lookup, so the other args still find the row.
	TEST( SubscriptionTests, NullArgDoesNotDropTheNotification ){
		constexpr sv target{ "subNullArg" };
		createResource( target );
		let ids = resourceIds( target ); ASSERT_EQ( ids.size(), 1u );

		auto listener = listen();
		deleteResource( target, ", created:$missing" );
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
}

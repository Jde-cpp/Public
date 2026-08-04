//The registry app-review2 #15 turns on: what the process asked to be subscribed to, as opposed to what the current
//socket happens to be carrying.  Clear() drops the second on every close - server-side subscriptions really do die with
//the socket - so the first has to outlive it or a reconnect has nothing to put back.
//Replay() itself needs a live session, so it is not driven here; what is, is the bookkeeping it reads.
#include <gtest/gtest.h>
#include <jde/ql/types/Subscription.h>
#include <jde/app/client/clientSubscriptions.h>
#include "helpers.h"//table(): TableQL has no default ctor.

#define let const auto

namespace Jde::App::Tests{

	struct Listener final : QL::IListener{
		Listener( str name )ι:IListener{name}{}
		α OnChange( const jvalue&, QL::SubscriptionId )ε->void override{}
		α OnTraces( App::Proto::FromServer::Traces&& )ι->void override{}
	};

	struct ClientSubscriptionsTests : ::testing::Test{
	protected:
		//_requests is a file-global, so each test starts by emptying it of whatever the last one left.
		α SetUp()->void override{ Forget(); _a = ms<Listener>("a"); _b = ms<Listener>("b"); }
		α TearDown()->void override{ Forget(); }
		α Forget()ι->void{
			for( let& r : Client::Subscriptions::Remembered() )
				Client::Subscriptions::Forget( r.Listener, {} );
		}
		α Remember( sp<Listener> listener, sv query, flat_set<QL::SubscriptionId> ids )ι->void{
			Client::Subscriptions::Remember( string{query}, jobject{{"id", 1}}, move(listener), ids );
		}
		α Queries()ι->vector<string>{
			vector<string> y;
			for( let& r : Client::Subscriptions::Remembered() )
				y.push_back( r.Query );
			return y;
		}
		sp<Listener> _a, _b;
	};

	TEST_F( ClientSubscriptionsTests, RemembersPerRequest ){
		Remember( _a, "subscription A{ a{id} }", {1} );
		Remember( _b, "subscription B{ b{id} }", {2} );
		EXPECT_EQ( Queries().size(), 2u );
	}

	//The reason a replay is safe to run repeatedly: it re-issues the same query, gets fresh ids, and must update the entry
	//rather than add a second one - which would then be replayed twice, and deliver every event twice.
	TEST_F( ClientSubscriptionsTests, ReplayingUpdatesIdsInPlace ){
		Remember( _a, "subscription A{ a{id} }", {1} );
		Remember( _a, "subscription A{ a{id} }", {99} );//same request, new session's ids.
		let remembered = Client::Subscriptions::Remembered();
		ASSERT_EQ( remembered.size(), 1u ) << "a replayed request was recorded twice";
		EXPECT_EQ( remembered.front().Ids, (flat_set<QL::SubscriptionId>{99}) );
	}

	//Same query text, different listener, is a different subscription - both have to come back.
	TEST_F( ClientSubscriptionsTests, SameQueryDifferentListenersAreDistinct ){
		Remember( _a, "subscription A{ a{id} }", {1} );
		Remember( _b, "subscription A{ a{id} }", {2} );
		EXPECT_EQ( Client::Subscriptions::Remembered().size(), 2u );
	}

	//#15's core invariant: Clear() is about the live socket, not about what was asked for.
	TEST_F( ClientSubscriptionsTests, ClearDoesNotForgetRequests ){
		Remember( _a, "subscription A{ a{id} }", {1} );
		Client::Subscriptions::Clear();//what CloseTasks does on every socket close.
		EXPECT_EQ( Client::Subscriptions::Remembered().size(), 1u ) << "the close dropped the request, so a reconnect has nothing to replay";
	}

	TEST_F( ClientSubscriptionsTests, ForgetByListenerDropsAll ){
		Remember( _a, "subscription A{ a{id} }", {1} );
		Remember( _a, "subscription B{ b{id} }", {2} );
		Remember( _b, "subscription C{ c{id} }", {3} );
		Client::Subscriptions::Forget( _a, {} );
		EXPECT_EQ( Queries(), (vector<string>{"subscription C{ c{id} }"}) );
	}

	//An unsubscribe must take the request with it, or the next reconnect puts back what was just cancelled.  A request
	//only goes when every id it produced is gone - one query can ack into several.
	TEST_F( ClientSubscriptionsTests, ForgetByIdNeedsEveryIdOfTheRequest ){
		Remember( _a, "subscription A{ a{id} }", {1,2} );
		Client::Subscriptions::Forget( _a, {1} );
		ASSERT_EQ( Client::Subscriptions::Remembered().size(), 1u ) << "dropped while one of its subscriptions was still live";
		EXPECT_EQ( Client::Subscriptions::Remembered().front().Ids, (flat_set<QL::SubscriptionId>{2}) );
		Client::Subscriptions::Forget( _a, {2} );
		EXPECT_TRUE( Client::Subscriptions::Remembered().empty() );
	}

	//M8: OnWebsocketReceive/OnTraces invoked their listeners while holding the shared_lock, so a listener that unsubscribes
	//in response to an event re-entered StopListenRemote - which takes the same mutex exclusively - and hung.
	//The re-entrant call runs on another thread, not this one, for two reasons: a same-thread relock of a shared_mutex is
	//UB rather than a guaranteed hang, and this way a regression fails on the deadline instead of wedging the suite.  The
	//detached thread completes harmlessly once the outer call returns and releases the lock.
	struct UnsubscribingListener final : QL::IListener{
		UnsubscribingListener()ι:IListener{"unsubscribes-on-change"}{}
		α OnChange( const jvalue&, QL::SubscriptionId )ε->void override{
			std::thread{ [this]{
				if( auto self = Self.lock(); self )//weak, not shared: a listener holding itself is a cycle, and LSan says so.
					Client::Subscriptions::StopListenRemote( self, {} );
				Unsubscribed = true;//last touch of `this` - the test only returns once it sees this.
			} }.detach();
			for( uint i=0; i<200 && !Unsubscribed; ++i )
				std::this_thread::sleep_for( 10ms );
		}
		α OnTraces( App::Proto::FromServer::Traces&& )ι->void override{}
		wp<QL::IListener> Self;//what StopListenRemote matches on.
		atomic<bool> Unsubscribed{};
	};

	TEST_F( ClientSubscriptionsTests, AListenerMayUnsubscribeFromItsOwnCallback ){
		auto listener = ms<UnsubscribingListener>();
		listener->Self = listener;
		QL::Subscription sub{ "tests", QL::EMutationQL::Create, table("tests") };
		sub.Id = 4242;
		Client::Subscriptions::ListenRemote( listener, move(sub) );
		Client::Subscriptions::OnWebsocketReceive( jobject{{"x",1}}, 4242 );
		EXPECT_TRUE( listener->Unsubscribed.load() ) << "the callback ran with _mutex held, so the unsubscribe could not take it";
	}

	//Forgetting one listener's ids must not touch another's request that happens to share an id value.
	TEST_F( ClientSubscriptionsTests, ForgetIsScopedToItsListener ){
		Remember( _a, "subscription A{ a{id} }", {1} );
		Remember( _b, "subscription B{ b{id} }", {1} );
		Client::Subscriptions::Forget( _a, {1} );
		EXPECT_EQ( Queries(), (vector<string>{"subscription B{ b{id} }"}) );
	}
}

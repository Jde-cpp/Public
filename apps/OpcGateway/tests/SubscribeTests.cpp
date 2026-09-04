#include <jde/fwk/process/execution.h>
#include <jde/fwk/utils/Stopwatch.h>
#include "../src/GatewayAppClient.h"
#include "../src/async/ConnectAwait.h"
#include "../src/UAClient.h"
#include "../src/types/UAClientException.h"
#include "utils/GatewayClientSocket.h"
#include "../src/types/proto/opc.FromServer.h"
#include "utils/ITest.h"
#include "../src/auth/OpcServerSession.h"

#define let const auto

namespace Jde::Opc::Gateway::Tests{
	using Jde::Web::Client::ClientSocketAwait;
	struct SubscribeTests : ITest{
		struct Listener final : IListener{
			Listener( SubscribeTests* tests )ι:_tests{ tests }{}
			α OnData( string opcId, NodeId nodeId, const vector<FromServer::Value>& values )ι->void override{
				TRACE( "OnData: opcId: '{}', nodeId: {}, valueCount: {}. 0={}", opcId, nodeId.ToString(), values.size(), values.size() ? std::to_string(values[0].of_case()) : "n/a" );
				ASSERT( values.size()==1 );
				if( values.size()==1 ){
					try{//ι: AsNumber throws for a non-finite or out-of-range reading (review3 #13), and this override may not let one out.
						auto v = FromServer::ToValue( values[0] );
						_tests->_value = v.AsNumber<uint>();
						TRACE( "Value updated to {}.", _tests->_value.load() );
					}
					catch( const std::exception& e ){
						WARN( "OnData: could not convert value for {}: {}", nodeId.ToString(), e.what() );
					}
				}
			}
		private:
			SubscribeTests* _tests;
		};
		Ω SetUpTestCase()ε->void{
			ITest::SetUpTestCase();
			optional<ssl::context> ctx;
			_session = ms<GatewayClientSocket>( Executor(), ctx );
			BlockVoidAwait( _session->RunSession("localhost", GatewayPort()) );
			BlockAwait<ClientSocketAwait<uint32>,uint>( _session->Connect(AppClient()->SessionId()) );
		}
		α SetUp()ι->void{
			_listener = ms<Listener>( this );
		}
	protected:
		atomic<uint> _value;
		sp<Listener> _listener;
		static sp<GatewayClientSocket> _session;
	};
	sp<GatewayClientSocket> SubscribeTests::_session;

	Ω read( sp<UAClient> client, NodeId nodeId )ε->uint{
		auto v = BlockTAwait<flat_map<NodeId, Value>>( ReadValueAwait{{nodeId}, client} ).at( nodeId );
		THROW_IFX( v.status, UAClientException(v.status, client->Handle()) );
		auto j = v.ToJson();
		TRACET( ELogTags::Test, "Initial value: {}.", serialize(j) );
		if( j.is_object() && j.as_object().contains("value") )
			j = j.as_object().at("value");
		let expected = v.ToJson().to_number<uint>();
		return expected;
	}
	TEST_F( SubscribeTests, Basic ){
		const NodeId nodeId{ 4, 6017 };
		let expected = read( _client, nodeId );
		BlockAwait<ClientSocketAwait<FromServer::SubscriptionAck>,FromServer::SubscriptionAck>( _session->Subscribe(OpcServerTarget, {nodeId}, _listener) );
		Stopwatch sw;
		while( _value!=expected ){
			ASSERT_NO_THROW( sw.CheckTimeout(6s, 1ms) );
		}
		string q = "updateVariable( opc: $opc, id: $id, value: $value ){ value }";
		let newValue = _value + 1;
		const jobject vars{ {"opc", OpcServerTarget}, {"id", nodeId.ToJson()}, {"value", newValue} };
		let json = BlockAwait<ClientSocketAwait<jvalue>,jvalue>( _session->Query(move(q), vars, true) );
		TRACE( "write result: {}", serialize(json) );
		sw.Reset();
		while( _value!=newValue ){
			ASSERT_NO_THROW( sw.CheckTimeout(6s, 1ms) );
		}
		ASSERT_EQ( newValue, json.as_object().at("value").to_number<uint>() );
		auto result = BlockAwait<ClientSocketAwait<FromServer::UnsubscribeAck>,FromServer::UnsubscribeAck>( _session->Unsubscribe(OpcServerTarget, {nodeId}) );
		ASSERT_TRUE( result.successes_size()==1 );
		TRACE( "-------------------------------------------------------------" );
		//teardown costs the gateway's 1s subscription wait + a 500ms poll tick, so poll rather than fixed-sleep.
		sw.Reset();
		while( _client->Processing() )
			ASSERT_NO_THROW( sw.CheckTimeout(6s, 1ms) );
	}

	//Unsubscribe looked the client up by the credential cached at connect, so anything that emptied the cache between
	//Subscribe and Unsubscribe - a logout, or an anonymous session, which is never cached - stranded the subscription as
	//"Client not found" (soak-findings #5).  It now derives the credential the way Subscribe did (SessionCredential).
	TEST_F( SubscribeTests, UnsubscribeSurvivesCredentialCacheLoss ){
		const NodeId nodeId{ 4, 6017 };
		let expected = read( _client, nodeId );
		BlockAwait<ClientSocketAwait<FromServer::SubscriptionAck>,FromServer::SubscriptionAck>( _session->Subscribe(OpcServerTarget, {nodeId}, _listener) );
		Stopwatch sw;//let the initial push land first - the listener writes into this fixture, so nothing may still be in flight when the test returns.
		while( _value!=expected )
			ASSERT_NO_THROW( sw.CheckTimeout(6s, 1ms) );
		let cached = GetCredential( AppClient()->SessionId(), OpcServerTarget );
		ASSERT_TRUE( cached );//the suite's shared state: later tests connect through this credential, so it goes back below.
		Logout( AppClient()->SessionId() );//drops the web session's cached credentials; the UA client itself stays connected.
		auto result = BlockAwait<ClientSocketAwait<FromServer::UnsubscribeAck>,FromServer::UnsubscribeAck>( _session->Unsubscribe(OpcServerTarget, {nodeId}) );
		EXPECT_EQ( result.successes_size(), 1 );
		AddSession( AppClient()->SessionId(), OpcServerTarget, *cached );//or the next connect derives the fallback credential and builds a second client beside _client.
		sw.Reset();//as Basic: the monitored item must be gone before the test returns.
		while( _client->Processing() )
			ASSERT_NO_THROW( sw.CheckTimeout(6s, 1ms) );
	}

	//A tab that closes or reloads sends no Unsubscribe frame.  The gateway's session *is* the Subscription's IDataChange,
	//so unless OnClose unsubscribes it the sp to the dead session, the UAClient it pins and the server-side monitored
	//items all survive every reload (review3 #4).
	TEST_F( SubscribeTests, CloseUnsubscribes ){
		const NodeId nodeId{ 4, 6017 };
		let before = _client->MonitoredNodes().Count();
		optional<ssl::context> ctx;
		auto session = ms<GatewayClientSocket>( Executor(), ctx );
		BlockVoidAwait( session->RunSession("localhost", GatewayPort()) );
		BlockAwait<ClientSocketAwait<uint32>,uint>( session->Connect(AppClient()->SessionId()) );
		BlockAwait<ClientSocketAwait<FromServer::SubscriptionAck>,FromServer::SubscriptionAck>( session->Subscribe(OpcServerTarget, {nodeId}, _listener) );
		ASSERT_EQ( _client->MonitoredNodes().Count(), before+1 );

		BlockVoidAwait( session->Close(false, SRCE_CUR) );
		Stopwatch sw;//the item goes away a DeleteMonitoring timer (1s) after the close, so poll rather than fixed-sleep.
		while( _client->MonitoredNodes().Count()!=before )
			ASSERT_NO_THROW( sw.CheckTimeout(10s, 1ms) );
	}

	//Sessions racing to connect to the same target coalesce in ConnectAwait::_requests, and a Create() failure fans the
	//one exception out to all of them.  The fan-out "cloned" with e.Move(), which moves the payload *out of* e, so every
	//waiter after the first - the last, which takes e itself, included - got the format string with its arguments gone
	//("Could not find connection:  '{}'") instead of the real message (review3 #5).
	//NB this only bites when Create() actually suspends before it throws, i.e. when ServerCnnctnAwait's SelectAsync is
	//async.  Under the ctest sqlite config it completes inline, Create runs to completion inside the first Suspend(), and
	//the fan-out only ever sees one handle - so a green run here is not by itself evidence of coverage.  Verified against
	//the bug by forcing a suspension into Create (a 200ms Any(DurationTimer)), which makes the fan-out see all three.
	TEST( ConnectCoalesceTests, EveryWaiterGetsTheMessage ){
		constexpr uint count{ 3 };
		struct Results{ atomic<uint> Done{}; std::array<string,count> What; };
		auto results = ms<Results>();
		auto connect = []( sp<Results> r, uint i )ι->TAwait<sp<UAClient>>::Task{
			try{
				co_await ConnectAwait{ "claudeNoSuchOpcTarget"s, Credential{} };
				r->What[i] = "<no exception>";
			}
			catch( runtime_error& e ){
				r->What[i] = e.what();
			}
			++r->Done;
		};
		for( uint i=0; i<count; ++i )
			connect( results, i );
		Stopwatch sw;
		while( results->Done<count )
			ASSERT_NO_THROW( sw.CheckTimeout(10s, 1ms) );
		for( uint i=0; i<count; ++i )
			TRACET( ELogTags::Test, "waiter {}: '{}'", i, results->What[i] );//not TRACE: _tags here is ITest's, and this is not a fixture test.
		for( uint i=0; i<count; ++i ){
			ASSERT_FALSE( results->What[i].empty() ) << "waiter " << i << " got a message-less exception.";
			ASSERT_EQ( results->What[i], results->What[0] ) << "waiter " << i << " got a different exception than waiter 0.";
		}
	}
}
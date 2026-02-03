#include <jde/fwk/process/execution.h>
#include <jde/fwk/utils/Stopwatch.h>
#include "../src/GatewayAppClient.h"
#include "../src/async/ConnectAwait.h"
#include "../src/UAClient.h"
#include "utils/GatewayClientSocket.h"
#include "../src/types/proto/opc.FromServer.h"
#include "utils/ITest.h"

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
					auto v = FromServer::ToValue( values[0] );
					_tests->_value = v.AsNumber<uint>();
					TRACE( "Value updated to {}.", _tests->_value.load() );
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

	α read( sp<UAClient> client, NodeId nodeId )ι->uint{
		auto v = BlockTAwait<flat_map<NodeId, Value>>( ReadValueAwait{{nodeId}, move(client)} ).at( nodeId );
		auto j = v.ToJson();
		TRACET( ELogTags::Test, "Initial value: {}.", serialize(j) );
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
		std::this_thread::sleep_for( 2s );// Gateway waits for 1 seconds before destroying the subscription.
		ASSERT_FALSE( _client->Processing() );
	}
}
#include <jde/web/Jwt.h>
#include <jde/web/client/socket/ClientSocketAwait.h>
#include "../src/GatewayAppClient.h"
#include "../src/UAClient.h"
#include "../src/ql/GatewayQL.h"
#include "../src/ql/NodeQLAwait.h"
#include "utils/helpers.h"
#define let const auto

namespace Jde::Opc::Gateway::Tests{
	constexpr ELogTags _tags{ ELogTags::Test };
	struct BrowseTests : ::testing::Test{
	protected:
		Ω SetUpTestCase()ε->void{
			_jwt = BlockAwait<Web::Client::ClientSocketAwait<Jde::Web::Jwt>,Web::Jwt>( AppClient()->Jwt() );
			auto sessionId = *Str::TryTo<SessionPK>(_jwt->SessionId, nullptr, 16);
			TRACE( "UserPK: {:x}, SessionId: {:x}", _jwt->UserPK.Value, sessionId );
			auto con = GetConnection( OpcServerTarget );
			Credential cred{ _jwt->Payload() };
			_client = BlockAwait<TAwait<sp<UAClient>>,sp<UAClient>>( ConnectAwait{move(con.Target), cred} );
			AddSession( sessionId, OpcServerTarget, move(cred) );
		};
		Ω TearDownTestCase()ι->void{
			if( _client )
				UAClient::RemoveClient( move(_client) );
		}
		α SetUp()ι->void{}

		static optional<Web::Jwt> _jwt;
		static sp<UAClient> _client;
	};
	optional<Web::Jwt> BrowseTests::_jwt;
	sp<UAClient> BrowseTests::_client;

	TEST_F( BrowseTests, NodeId ){
		auto query = "node( opc: $opc, path:$path ){ id name parents{id name path} }";
		jobject variables{ {"opc", OpcServerTarget}, {"path", "4~Examples/4~Stacklights/4~ExampleStacklight/4~Lamp1"} };
		auto ql = QL::Parse( move(query), move(variables), Schemas(), true );
		auto value = BlockAwait<NodeQLAwait, jvalue>( NodeQLAwait{move(ql.Queries().front()), _client} );
		TRACE( "value: {}", serialize(value) );
		auto result = ExNodeId{ value };
		ASSERT_TRUE( *result.Numeric()>0 );
		ASSERT_EQ( value.at("name"), "Lamp1" );
	}
}
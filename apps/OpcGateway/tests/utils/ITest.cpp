#include "ITest.h"
#include "../../src/GatewayAppClient.h"
#include "../../src/async/ConnectAwait.h"
#include "../../src/UAClient.h"
#include "helpers.h"

#define let const auto

namespace Jde::Opc::Gateway::Tests{
	α ITest::SetUpTestCase()ι->void{
		_jwt = BlockAwait<Web::Client::ClientSocketAwait<Jde::Web::Jwt>,Web::Jwt>( AppClient()->Jwt() );
		let sessionId = *Str::TryTo<SessionPK>(_jwt->SessionId, nullptr, 16);
		TRACE( "UserPK: {:x}, SessionId: {:x}", _jwt->UserPK.Value, sessionId );
		auto con = GetConnection( OpcServerTarget );
		Credential cred{ _jwt->Payload() };
		_client = BlockTAwait<sp<UAClient>>( ConnectAwait{move(con.Target), cred} );
		AddSession( sessionId, OpcServerTarget, move(cred) );
	}
	α ITest::TearDownTestCase()ι->void{
		UAClient::RemoveClient( move(_client) );
	}

	optional<Web::Jwt> ITest::_jwt;
	sp<UAClient> ITest::_client;
}
#include <jde/web/Jwt.h>
#include <jde/web/client/socket/ClientSocketAwait.h>
#include "../src/GatewayAppClient.h"
#include "../src/UAClient.h"
#include "../src/ql/GatewayQL.h"
#include "../src/ql/NodeQLAwait.h"
#include "utils/helpers.h"
#include "../src/types/UAClientException.h"
#define let const auto

namespace Jde::Opc::Gateway::Tests{
	constexpr ELogTags _tags{ ELogTags::Test };
	struct BrowseTests : ::testing::Test{
	protected:
		Ω SetUpTestCase()ε->void{
			try{
				_jwt = BlockAwait<Web::Client::ClientSocketAwait<Jde::Web::Jwt>,Web::Jwt>( AppClient()->Jwt() );
				auto sessionId = *Str::TryTo<SessionPK>(_jwt->SessionId, nullptr, 16);
				TRACE( "UserPK: {:x}, SessionId: {:x}", _jwt->UserPK.Value, sessionId );
				auto con = GetConnection( OpcServerTarget );
				Credential cred{ _jwt->Payload() }; cred.SetUserPK( _jwt->UserPK );
				_client = BlockAwait<TAwait<sp<UAClient>>,sp<UAClient>>( ConnectAwait{move(con.Target), cred} );
				AddSession( sessionId, OpcServerTarget, move(cred) );
			}
			catch( runtime_error& e ){
				INFOT( ELogTags::Test, "Failed to connect to gateway: {}", e.what() );
				THROW( "Failed to connect to gateway: {}", e.what() );
			}
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

	//UserMessage() is the client-facing text.  Its combine branch tested _userMessage inside `if( _userMessage.empty() )`,
	//so it was dead and a 3-arg exception returned the bare description with the UA status stripped (review3 #14).  what()
	//is checked alongside to pin down why reviving that branch was not the fix:  ExternalException has already folded the
	//description into it, so combining with the base message would have repeated the description instead.
	TEST( UAClientExceptionTests, UserMessageCarriesStatusAndDescription ){
		const UAClientException status{ (StatusCode)UA_STATUSCODE_BADNODEIDUNKNOWN, Jde::Handle{0xABC}, RequestId{0xDEF} };
		ASSERT_EQ( status.ClientDetail(), "(80340000)BadNodeIdUnknown" );
		ASSERT_EQ( status.UserMessage(), "(80340000)BadNodeIdUnknown" ) << "no description, so the status alone - and never what()'s [handle.requestId] prefix";
		ASSERT_EQ( string{status.what()}, "[abc.def](80340000)BadNodeIdUnknown" );

		const UAClientException described{ (StatusCode)UA_STATUSCODE_BADNODEIDUNKNOWN, Jde::Handle{0xABC}, string{"applicationUri mismatch"} };
		ASSERT_EQ( described.UserMessage(), "(80340000)BadNodeIdUnknown - applicationUri mismatch" ) << "the status must survive alongside the description";
		ASSERT_EQ( string{described.what()}, "(80340000)BadNodeIdUnknown - applicationUri mismatch" );
	}

	//A server may answer a browse with a GOOD serviceResult and resultsSize==0 (results==nullptr);  FoldersAwait::OnComplete
	//treats that as success, and the QL path then visits it.  VisitWhile's only bounds check was an ASSERT_DESC, which
	//merely logs and carries on into results[0] (review3 #13).
	TEST( BrowseResponseTests, VisitWhileToleratesAnEmptyResult ){
		Browse::Response response{ UA_BrowseResponse{} };
		ASSERT_EQ( response.resultsSize, 0u );
		uint visits{};
		ASSERT_TRUE( response.VisitWhile(0, [&](const UA_ReferenceDescription&){ ++visits; return true; }) );
		ASSERT_EQ( visits, 0u );
	}

	//RemoveClient erased whatever occupied the (Target,Credential) key without checking it was the client being removed.
	//A stale second remove of A - the UAClientException ctor's BadServerNotConnected path feeds them - then evicted the
	//replacement B that had reconnected under the same key, leaving B connected but unreachable through Find, holding its
	//monitored items, NodeIndex and EnumTypeCache (review3 #12).
	TEST( RemoveClientTests, StaleRemoveKeepsTheReplacement ){
		let jwt = BlockAwait<Web::Client::ClientSocketAwait<Jde::Web::Jwt>,Web::Jwt>( AppClient()->Jwt() );
		Credential cred{ jwt.Payload() }; cred.SetUserPK( jwt.UserPK );//the only credential this server accepts - anonymous is BadIdentityTokenRejected.
		auto a = BlockTAwait<sp<UAClient>>( ConnectAwait{string{OpcServerTarget}, cred} );
		ASSERT_TRUE( a );
		ASSERT_TRUE( UAClient::RemoveClient(sp<UAClient>{a}) );

		auto b = BlockTAwait<sp<UAClient>>( ConnectAwait{string{OpcServerTarget}, cred} );
		ASSERT_TRUE( b );
		ASSERT_NE( a, b ) << "expected a fresh client at the same key";
		ASSERT_EQ( UAClient::Find(OpcServerTarget, cred), b );

		UAClient::RemoveClient( sp<UAClient>{a} );//the stale double-remove of the predecessor
		ASSERT_EQ( UAClient::Find(OpcServerTarget, cred), b ) << "a stale remove of the predecessor evicted the replacement";
		UAClient::RemoveClient( move(b) );
	}

	//ReadRequest owns its node ids.  The test address space is all numeric, so a shallow UA_NodeId slice - which shares
	//identifier.string.data with a source the request outlives - shows up in no other test here; ASan reports the read
	//below as a heap-use-after-free without the deep copy (gateway-review3 #2).
	TEST( ReadRequestTests, OwnsNodeIdIdentifier ){
		let identifier = "a.string.identifier.long.enough.to.need.the.heap"s;
		//The temporary NodeId dies at the ';' - as the temporary vector<NodeId> does at NodeQLAwait.cpp:109, and as the
		//caller's Browse::Response does while the read is still in flight.
		ReadRequest request{ NodeId{UA_NODEID_STRING_ALLOC(2, "a.string.identifier.long.enough.to.need.the.heap")}, {UA_ATTRIBUTEID_VALUE} };
		ASSERT_EQ( request.nodesToReadSize, 1u );
		ASSERT_EQ( request.nodesToRead[0].nodeId.identifierType, UA_NODEIDTYPE_STRING );
		ASSERT_EQ( request.nodesToRead[0].nodeId.namespaceIndex, 2 );
		ASSERT_EQ( Opc::ToString(request.nodesToRead[0].nodeId.identifier.string), identifier );

		auto moved = move( request );//ReadAwait moves it in, then ReadResponse moves it again - neither may double-free.
		ASSERT_EQ( moved.nodesToReadSize, 1u );
		ASSERT_EQ( Opc::ToString(moved.nodesToRead[0].nodeId.identifier.string), identifier );
	}
}
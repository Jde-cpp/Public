#include "GatewaySocketSession.h"
#include <jde/app/client/IAppClient.h>
#include "StartupAwait.h"
#include "UAClient.h"
#include "WebServer.h"
#include "async/CreateSubscriptions.h"
#include "async/DataChanges.h"
#include "async/SessionAwait.h"
#include "auth/UM.h"
#include "types/proto/Opc.FromClient.h"
#include "types/proto/Opc.FromServer.h"
#include "uatypes/UAClientException.h"

#define let const auto

namespace Jde::Opc::Gateway{
	GatewaySocketSession::GatewaySocketSession( sp<RestStream> stream, beast::flat_buffer&& buffer, TRequestType&& request, tcp::endpoint&& userEndpoint, uint32 connectionIndex )ι:
		base{ move(stream), move(buffer), move(request), move(userEndpoint), connectionIndex }
	{}

	α GatewaySocketSession::OnClose()ι->void{
		LogRead( "OnClose", 0 );
		Server::RemoveSession( Id() );
		base::OnClose();
	}

	α GatewaySocketSession::Schemas()Ι->const vector<sp<DB::AppSchema>>&{
		return Gateway::Schemas();
	}
	α GatewaySocketSession::SendAck( uint32 id )ι->void{
		LogWrite( Ƒ("Ack id: {:x}", id), 0 );
		Write( FromServer::AckTrans(id) );
	}

	α GatewaySocketSession::SendDataChange( const ServerCnnctnNK& opcNK, const NodeId& node, const Value& value )ι->void{
		return Write( MessageTrans(FromServer::ToProto(opcNK,node, value), 0) );
	}

	α GatewaySocketSession::SetSessionId( str strSessionId, RequestId requestId )->Sessions::UpsertAwait::Task{
		LogRead( Ƒ("sessionId: '{}'", strSessionId), requestId );
		try{
			let sessionInfo = co_await Sessions::UpsertAwait( strSessionId, _userEndpoint.address().to_string(), true, AppClient() );
			base::SetSessionId( sessionInfo->SessionId );
			Write( FromServer::CompleteTrans(requestId) );
		}
		catch( IException& e ){
			WriteException( move(e), requestId );
		}
	}

	α GatewaySocketSession::Subscribe( ServerCnnctnNK&& opcId, flat_set<NodeId> nodes, uint32 requestId )ι->void{
		try{
			auto self = SharedFromThis(); //keep alive
			auto cred = GetCredential( base::SessionId(), opcId );
			LogRead( Ƒ("({:x})Subscribe: opcId: '{}', cred: '{}', nodeCount: {}", base::SessionId(), opcId, cred ? cred->ToString() : "null", nodes.size()), requestId );
			if( auto client = UAClient::Find(move(opcId), cred); client )
				CreateSubscription( client, move(nodes), requestId );
			else
				WriteException( Ƒ("Client not found: opcId: '{}'", opcId), requestId );
		}
		catch( IException& e ){
			WriteException( move(e), requestId );
		}
	}
	α GatewaySocketSession::CreateSubscription( sp<UAClient> client, flat_set<NodeId> nodes, uint32 requestId )ι->TAwait<sp<UA_CreateSubscriptionResponse>>::Task{
		try{
			auto self = SharedFromThis(); //keep alive
			co_await CreateSubscriptionAwait{ client };
			[]( sp<UAClient> client, flat_set<NodeId> nodes, uint32 requestId, sp<GatewaySocketSession> self )->TAwait<FromServer::SubscriptionAck>::Task{
				try{
					auto ack = co_await DataChangeAwait{ move(nodes), move(self), move(client) };
					self->Write( FromServer::SubscribeAckTrans(move(ack), requestId) );
				}
				catch( exception& e ){
					//if( !e.IsBadSession() )
						self->WriteException( move(e), requestId );
				//co_await AwaitSessionActivation( client );
				//ack = ( co_await DataChangesSubscribe(move(nodes), self, move(client)) ).UP<FromServer::SubscriptionAck>();
				}
			}( client, move(nodes), requestId, move(self) );
		}
		catch( exception& e ){
			WriteException( move(e), requestId );
		}
	}

	α GatewaySocketSession::Unsubscribe( ServerCnnctnNK&& opcId, flat_set<NodeId> nodes, uint32 requestId )ι->void {
		try{
			auto self = SharedFromThis();//keep alive
			auto cred = GetCredential( SessionId(), opcId );
			LogRead( Ƒ("Unsubscribe: opcId: '{}', user: '{}', nodeCount: {}", opcId, cred ? cred->ToString() : "null", nodes.size()), requestId );
			if( auto client = UAClient::Find(move(opcId), cred); client ){
				auto [successes,failures] = client->MonitoredNodes.Unsubscribe( move(nodes), self );
				Write( FromServer::UnsubscribeTrans(requestId, move(successes), move(failures)) );
			}
			else
				WriteException( Ƒ("Client not found: opcId: '{}'", opcId), requestId );
		}
		catch( IException& e ){
			WriteException( move(e), requestId );
		}
	}
	α GatewaySocketSession::WriteSubscription( const jvalue& /*j*/, Jde::RequestId /*requestId*/ )ι->void{
		ASSERT_DESC( false, "Not Implemented" );
	}
	α GatewaySocketSession::WriteSubscriptionAck( vector<QL::SubscriptionId>&& /*subscriptionIds*/, Jde::RequestId /*requestId*/ )ι->void{
		ASSERT_DESC( false, "Not Implemented" );
	}
	α GatewaySocketSession::WriteComplete( Jde::RequestId /*requestId*/ )ι->void{
		ASSERT_DESC( false, "Not Implemented" );
	}

	α GatewaySocketSession::WriteException( exception&& e, Jde::RequestId requestId )ι->void{
		LogWriteException( e, requestId );
		Write( FromServer::ExceptionTrans(move(e), requestId) );
	}
	α GatewaySocketSession::WriteException( string&& e, Jde::RequestId requestId )ι->void{
		LogWriteException( move(e), requestId );
		Write( FromServer::ExceptionTrans( Exception(move(e)), requestId ) );
	}

	α GatewaySocketSession::OnRead( FromClient::Transmission&& transmission )ι->void{
		for( auto i=0; i<transmission.messages_size(); ++i ){
			auto& m = *transmission.mutable_messages( i );
			let requestId = m.request_id();

			FromClient::Message msg;
			uint32 requestId2 = 0;
			string sessionId = "c6a22e5c";
			msg.set_request_id( requestId2 );
			msg.set_session_id( sessionId );
			let result = Jde::Proto::ToString( msg );

			switch( m.Value_case() ){
			using enum FromClient::Message::ValueCase;
			case kSessionId:
				SetSessionId( m.session_id(), requestId );
				break;
			case kSubscribe:{
				auto& s = *m.mutable_subscribe();
				Subscribe( move(*s.mutable_opc_id()), FromClientUtils::ToNodes(move(*s.mutable_nodes())), requestId );
				break;}
			case kUnsubscribe:{
				auto& u = *m.mutable_unsubscribe();
				Unsubscribe( move(*u.mutable_opc_id()), FromClientUtils::ToNodes(move(*u.mutable_nodes())), requestId );
				break;}
			default:
				LogRead( Ƒ("Unknown message type '{}'", underlying(m.Value_case())), requestId, ELogLevel::Critical );
				WriteException( Exception("({})Message not implemented."), requestId );
			}
		}
	}
}
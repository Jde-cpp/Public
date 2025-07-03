#include "ServerSocketSession.h"
#include <jde/opc/UM.h>
#include <jde/opc/async/CreateSubscriptions.h>
#include <jde/opc/async/DataChanges.h>
#include <jde/opc/async/SessionAwait.h>
#include "WebServer.h"
#include "types/proto/Opc.FromServer.h"
#include <jde/opc/uatypes/UAClientException.h>

#define let const auto

namespace Jde::Opc{
	ServerSocketSession::ServerSocketSession( sp<RestStream> stream, beast::flat_buffer&& buffer, TRequestType&& request, tcp::endpoint&& userEndpoint, uint32 connectionIndex )ι:
		base{ move(stream), move(buffer), move(request), move(userEndpoint), connectionIndex }
	{}

	α ServerSocketSession::OnClose()ι->void{
		LogRead( "OnClose", 0 );
		Server::RemoveSession( Id() );
		base::OnClose();
	}

	α ServerSocketSession::SendAck( uint32 id )ι->void{
		LogWrite( Ƒ("Ack id: {:x}", id), 0 );
		Write( FromServer::AckTrans(id) );
	}

	α ServerSocketSession::SendDataChange( const Jde::Opc::OpcClientNK& opcNK, const Jde::Opc::NodeId& node, const Jde::Opc::Value& value )ι->void{
		return Write( MessageTrans(value.ToProto(opcNK,node), 0) );
	}

	α ServerSocketSession::SetSessionId( str strSessionId, RequestId requestId )->Sessions::UpsertAwait::Task{
		LogRead( Ƒ("sessionId: '{}'", strSessionId), requestId );
		try{
			let sessionInfo = co_await Sessions::UpsertAwait( strSessionId, _userEndpoint.address().to_string(), true );
			base::SetSessionId( sessionInfo->SessionId );
			Write( FromServer::CompleteTrans(requestId) );
		}
		catch( IException& e ){
			WriteException( move(e), requestId );
		}
	}

	α ServerSocketSession::Subscribe( OpcClientNK&& opcId, flat_set<NodeId> nodes, uint32 requestId )ι->void{
		try{
			auto self = SharedFromThis(); //keep alive
			auto [loginName,password] = Credentials( base::SessionId(), opcId );
			LogRead( Ƒ("({:x})Subscribe: opcId: '{}', user: '{}', nodeCount: {}", base::SessionId(), opcId, loginName, nodes.size()), requestId );
			if( auto client = UAClient::Find( move(opcId), loginName, password); client )
				CreateSubscription( client, move(nodes), requestId );
			else
				WriteException( Ƒ("Client not found: opcId: '{}'", opcId), requestId );
		}
		catch( IException& e ){
			WriteException( move(e), requestId );
		}
	}
	α ServerSocketSession::CreateSubscription( sp<UAClient> client, flat_set<NodeId> nodes, uint32 requestId )ι->Task{
		try{
			auto self = SharedFromThis(); //keep alive
			( co_await Opc::CreateSubscription(client) ).CheckError();
			up<FromServer::SubscriptionAck> ack;
			try{
				ack = ( co_await DataChangesSubscribe(nodes, self, client) ).UP<FromServer::SubscriptionAck>();
			}
			catch( Client::UAClientException& e ){
				if( !e.IsBadSession() )
					e.Throw();
			}
			if( !ack ){
				co_await AwaitSessionActivation( client );
				ack = ( co_await DataChangesSubscribe(move(nodes), self, move(client)) ).UP<FromServer::SubscriptionAck>();
			}
			Write( FromServer::SubscribeAckTrans(move(ack), requestId) );
		}
		catch( IException& e ){
			WriteException( move(e), requestId );
		}
	}

	α ServerSocketSession::Unsubscribe( OpcClientNK&& opcId, flat_set<NodeId> nodes, uint32 requestId )ι->void{
		try{
			auto self = SharedFromThis();//keep alive
			auto [loginName,password] = Opc::Credentials( SessionId(), opcId );
			LogRead( Ƒ("Unsubscribe: opcId: '{}', user: '{}', nodeCount: {}", opcId, loginName, nodes.size()), requestId );
			if( auto pClient = UAClient::Find( move(opcId), loginName, password); pClient ){
				auto [successes,failures] = pClient->MonitoredNodes.Unsubscribe( move(nodes), self );
				Write( FromServer::UnsubscribeTrans(requestId, move(successes), move(failures)) );
			}
			else
				WriteException( Ƒ("Client not found: opcId: '{}'", opcId), requestId );
		}
		catch( IException& e ){
			WriteException( move(e), requestId );
		}
	}
	α ServerSocketSession::WriteSubscription( const jvalue& /*j*/, Jde::RequestId /*requestId*/ )ι->void{
		ASSERT_DESC( false, "Not Implemented" );
	}
	α ServerSocketSession::WriteSubscriptionAck( vector<QL::SubscriptionId>&& /*subscriptionIds*/, Jde::RequestId /*requestId*/ )ι->void{
		ASSERT_DESC( false, "Not Implemented" );
	}
	α ServerSocketSession::WriteComplete( Jde::RequestId /*requestId*/ )ι->void{
		ASSERT_DESC( false, "Not Implemented" );
	}

	α ServerSocketSession::WriteException( exception&& e, Jde::RequestId requestId )ι->void{
		LogWriteException( e, requestId );
		Write( FromServer::ExceptionTrans(move(e), requestId) );
	}
	α ServerSocketSession::WriteException( string&& e, Jde::RequestId requestId )ι->void{
		LogWriteException( move(e), requestId );
		Write( FromServer::ExceptionTrans( Exception(move(e)), requestId ) );
	}

	α ServerSocketSession::OnRead( FromClient::Transmission&& transmission )ι->void{
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
				Subscribe( move(*s.mutable_opc_id()), NodeId::ToNodes(move(*s.mutable_nodes())), requestId );
				break;}
			case kUnsubscribe:{
				auto& u = *m.mutable_unsubscribe();
				Unsubscribe( move(*u.mutable_opc_id()), NodeId::ToNodes(move(*u.mutable_nodes())), requestId );
				break;}
			default:
				LogRead( Ƒ("Unknown message type '{}'", underlying(m.Value_case())), requestId, ELogLevel::Critical );
				WriteException( Exception("({})Message not implemented."), requestId );
			}
		}
	}
}
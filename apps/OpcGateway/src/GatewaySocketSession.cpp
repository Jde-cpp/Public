#include "GatewaySocketSession.h"
#include <jde/app/proto/Common.pb.h>
#include <jde/app/client/IAppClient.h> //!important
#include "StartupAwait.h"
#include "UAClient.h"
#include "WebServer.h"
#include "async/Subscriptions.h"
#include "async/DataChanges.h"
#include "ql/GatewayQLAwait.h"
#include "types/proto/opc.Common.h"
#include "types/proto/opc.FromServer.h"

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

	α GatewaySocketSession::SendAck( uint32 id )ι->void{
		LogWrite( Ƒ("Ack id: {:x}", id), 0 );
		Write( FromServer::AckTrans(id) );
	}

	α GatewaySocketSession::SendDataChange( const ServerCnnctnNK& opcNK, const NodeId& node, const Value& value )ι->void{
		return Write( MessageTrans(FromServer::ToProto(opcNK, node, value, 0), 0) );
	}

	α GatewaySocketSession::SetSessionId( str strSessionId, RequestId requestId )->Sessions::UpsertAwait::Task{
		LogRead( Ƒ("sessionId: '{}'", strSessionId), requestId );
		try{
			let sessionInfo = co_await Sessions::UpsertAwait( strSessionId, _userEndpoint.address().to_string(), true, AppClient() );
			base::SetSessionInfo( move(sessionInfo) );
			Write( FromServer::CompleteTrans(requestId) );
		}
		catch( IException& e ){
			WriteException( move(e), requestId );
		}
	}

	α GatewaySocketSession::Subscribe( ServerCnnctnNK&& opcId, flat_set<NodeId> nodes, uint32 requestId )ι->TAwait<sp<UAClient>>::Task{
		try{
			auto self = SharedFromThis(); //keep alive
			LogRead( Ƒ("({:x})Subscribe: opcId: '{}', nodeCount: {}", base::SessionId(), opcId, nodes.size()), requestId );
			ASSERT( Session() );
			auto client = co_await ConnectAwait( string{opcId}, *Session() );
			if( client )
				CreateSubscription( move(client), move(nodes), requestId );
			else
				WriteException( Ƒ("Client not found: opcId: '{}'", move(opcId)), requestId );
		}
		catch( IException& e ){
			WriteException( move(e), requestId );
		}
	}

	Ω subscribe( sp<UAClient> client, flat_set<NodeId> nodes, RequestId requestId, sp<GatewaySocketSession> session )ι->TAwait<FromServer::SubscriptionAck>::Task{
		try{
			auto ack = co_await DataChangeAwait{ nodes, session, client };
			session->Write( FromServer::SubscribeAckTrans(move(ack), requestId) );
			[]( flat_set<NodeId>&& nodes, sp<UAClient> client, RequestId requestId, sp<GatewaySocketSession> session )->TAwait<flat_map<NodeId, Value>>::Task {
				try{
					auto values = co_await ReadValueAwait{ move(nodes), client };
					session->Write( FromServer::ReadValuesTrans(client->Target(), move(values), requestId) );
				}
				catch( IException& e ){
					session->WriteException( move(e), requestId );
				}
			}( move(nodes), move(client), requestId, move(session) );
		}
		catch( exception& e ){
			session->WriteException( move(e), requestId );
		}
	}
	α GatewaySocketSession::CreateSubscription( sp<UAClient> client, flat_set<NodeId> nodes, RequestId requestId )ι->VoidAwait::Task{
		try{
			auto self = SharedFromThis(); //keep alive
			co_await SubscribeAwait{ client };
			subscribe( move(client), move(nodes), requestId, move(self) );
		}
		catch( exception& e ){
			WriteException( move(e), requestId );
		}
	}

	α GatewaySocketSession::GraphQL( Jde::Proto::Query&& proto, uint requestId )ι->TAwait<jvalue>::Task{
		jobject vars;
		try{
			if( auto p = proto.mutable_variables(); p->size() )
				vars = parse( move(*p) ).as_object();
			auto ql = QL::Parse( move(*proto.mutable_text()), move(vars), Schemas(), proto.return_raw() );
			auto v = co_await GatewayQLAwait{ move(ql), Session(), proto.return_raw() };
			Write( FromServer::QueryTrans(serialize(move(v)), requestId) );
		}
		catch( exception& e ){
			WriteException( move(e), requestId );
			co_return;
		}
	}
	α GatewaySocketSession::Unsubscribe( ServerCnnctnNK&& opcId, flat_set<NodeId> nodes, uint32 requestId )ι->void {
		try{
			auto self = SharedFromThis();//keep alive
			auto cred = GetCredential( SessionId(), opcId );
			LogRead( Ƒ("Unsubscribe: opcId: '{}', user: '{}', nodeCount: {}", opcId, cred ? cred->ToString() : "null", nodes.size()), requestId );
			if( auto client = cred ? UAClient::Find(move(opcId), *cred) : nullptr; client ){
				auto [successes,failures] = client->MonitoredNodes().Unsubscribe( move(nodes), self );
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
	α GatewaySocketSession::WriteSubscriptionAck( flat_set<QL::SubscriptionId>&& /*subscriptionIds*/, Jde::RequestId /*requestId*/ )ι->void{
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
		Write( FromServer::ExceptionTrans(Exception(move(e)), requestId) );
	}

	α GatewaySocketSession::OnRead( FromClient::Transmission&& transmission )ι->void{
		for( auto i=0; i<transmission.messages_size(); ++i ){
			auto& m = *transmission.mutable_messages( i );
			let requestId = m.request_id();

			switch( m.Value_case() ){
			using enum FromClient::Message::ValueCase;
			case kSessionId:
				SetSessionId( m.session_id(), requestId );
				break;
			case kSubscribe:{
				auto& s = *m.mutable_subscribe();
				Subscribe( move(*s.mutable_opc_id()), ProtoUtils::ToNodeIds(move(*s.mutable_nodes())), requestId );
				break;}
			case kQuery:
				GraphQL( move(*m.mutable_query()), requestId );
				break;
			case kUnsubscribe:{
				auto& u = *m.mutable_unsubscribe();
				Unsubscribe( move(*u.mutable_opc_id()), ProtoUtils::ToNodeIds(move(*u.mutable_nodes())), requestId );
				break;}
			default:
				LogRead( Ƒ("Unknown message type '{}'", underlying(m.Value_case())), requestId, ELogLevel::Critical );
				WriteException( Exception("({})Message not implemented."), requestId );
			}
		}
	}
}
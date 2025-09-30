#include "ServerSocketSession.h"
#include <jde/app/shared/proto/App.FromServer.h>
#include <jde/app/shared/StringCache.h>
#include <jde/app/shared/proto/App.FromClient.h>
#include <jde/fwk/chrono.h>
#include <jde/access/server/accessServer.h>
#include "LogData.h"
#include "WebServer.h"
#include "ServerSocketSession.h"
#define let const auto

namespace Jde::App::Server{
	α ToProto( const Web::Server::SessionInfo& session, RequestId requestId )ι->Proto::FromServer::Transmission;

	ServerSocketSession::ServerSocketSession( sp<RestStream> stream, beast::flat_buffer&& buffer, TRequestType&& request, tcp::endpoint&& userEndpoint, uint32 connectionIndex )ι:
		base{ move(stream), move(buffer), move(request), move(userEndpoint), connectionIndex }
	{}

	α ServerSocketSession::AddInstance( Proto::FromClient::Instance instance, RequestId requestId )ι->TAwait<sp<Web::Server::SessionInfo>>::Task{
		try{
			auto info = co_await Web::Server::Sessions::UpsertAwait( Ƒ("{:x}", instance.session_id()), _userEndpoint.address().to_string(), true, nullptr );
			_userPK = info->UserPK;
			base::SetSessionId( instance.session_id() );
			let [appPK,instancePK] = App::AddInstance( instance.application(), instance.host(), instance.pid() );//TODO Don't block
			INFOT( ELogTags::SocketServerRead, "[{:x}.{:x}]Adding application app:{}@{}:{} pid:{}, instancePK:{:x}, sessionId: {:x}, endpoint: '{}'", Id(), requestId, instance.application(), instance.host(), instance.web_port(), instance.pid(), instancePK, instance.session_id(), _userEndpoint.address().to_string() );
			Server::RemoveExisting( instance.host(), instance.web_port() );
			_instancePK = instancePK; _appPK = appPK;
			_instance = move( instance );
			Write( FromServer::ConnectionInfo(appPK, instancePK, requestId, AppClient()->PublicKey()) );
		}
		catch( exception& e ){
			WriteException( move(e), requestId );
		}
	}
	α ServerSocketSession::AddSession( Proto::FromClient::AddSession m, RequestId requestId, SL /*sl*/ )ι->TAwait<Jde::UserPK>::Task{
		let _ = shared_from_this();
		try{
			LogRead( Ƒ("AddSession user: '{}', endpoint: '{}', provider: {}, is_socket: {}", m.domain()+"/"+m.login_name(), m.user_endpoint(), m.provider_pk(), m.is_socket()), requestId );
			let userPK = co_await Access::Server::Authenticate(m.login_name(), m.provider_pk(), m.domain());

			auto sessionInfo = Web::Server::Sessions::Add( userPK, move(*m.mutable_user_endpoint()), m.is_socket() );
			LogWrite( Ƒ("AddSession id: {:x}", sessionInfo->SessionId), requestId );
			Write( ToProto(*sessionInfo, requestId) );
		}
		catch( IException& e ){
			WriteException( move(e), requestId );
		}
	}
	α ServerSocketSession::Execute( string&& bytes, optional<Jde::UserPK> userPK, RequestId clientRequestId )ι->void{
		try{
			auto t = Jde::Proto::Deserialize<Proto::FromClient::Transmission>( move(bytes) );
			ProcessTransmission( move(t), userPK, clientRequestId );
		}
		catch( IException& e ){
			WriteException( move(e), clientRequestId );
		}
	}

	α ServerSocketSession::ForwardExecution( Proto::FromClient::ForwardExecution&& m, bool anonymous, RequestId requestId, SL sl )ι->ForwardExecutionAwait::Task{
		sv functionSuffix = anonymous ? "Anonymous" : "";
		LogRead( Ƒ("ForwardExecution{} appPK: {}, appInstancePK: {:x}, size: {:10L}", functionSuffix, m.app_pk(), m.app_instance_pk(), m.execution_transmission().size()), requestId );
		try{
			string result = co_await ForwardExecutionAwait{ _userPK.value_or(Jde::UserPK{0}), move(m), SharedFromThis(), sl };
			LogWrite( Ƒ("ForwardExecution{} size: {:10L}", functionSuffix, result.size()), requestId );
			Write( FromServer::Execute(move(result), requestId) );
		}
		catch( IException& e ){
			WriteException( move(e), requestId );
		}
	}
	α ServerSocketSession::GraphQL( string&& query, bool returnRaw, RequestId requestId )ι->QL::QLAwait<jvalue>::Task{
		let _ = shared_from_this();
		try{
			LogRead( Ƒ("GraphQL{}: {}", returnRaw ? "*" : "", query), requestId );
			auto j = co_await QL::QLAwait( move(query), _userPK.value_or(Jde::UserPK{0}), Server::Schemas(), returnRaw );
			auto y = serialize( j );
			LogWrite( Ƒ("GraphQL: {}", y.substr(0,100)), requestId );
			Write( FromServer::GraphQL(move(y), requestId) );
		}
		catch( IException& e ){
			WriteException( move(e), requestId );
		}
	}
	α ServerSocketSession::Schemas()Ι->const vector<sp<DB::AppSchema>>&{
		return Server::Schemas();
	}

	α ServerSocketSession::SaveLogEntry( Log::Proto::LogEntryClient entry, RequestId requestId )->void{
		if( !_appPK || !_instancePK ){
			WriteException( Exception{"ApplicationId or InstanceId not set.", ELogLevel::Warning}, requestId );
			return;
		}
		let level = (ELogLevel)entry.level();
		vector<string> args = Jde::Proto::ToVector( move(*entry.mutable_args()) );
		// if( _dbLevel!=ELogLevel::NoLog && _dbLevel<=level )
		// 	SaveMessage( _appPK, _instancePK, entry );//TODO don't block
		if( _webLevel!=ELogLevel::NoLog && _webLevel<=level ){
			Logging::Entry y{ App::FromClient::FromLogEntry(move(entry)) };
			y.Text = StringCache::GetMessage( y.Id() );
			y.SetFile( StringCache::GetFile(y.FileId()) );
			y.SetFunction( StringCache::GetFunction(y.FunctionId()) );
			Server::BroadcastLogEntry( 0, _appPK, _instancePK, y, move(args) );
		}
	}
	α ServerSocketSession::SendAck( uint32 id )ι->void{
		Write( FromServer::Ack(id) );
	}

	α ServerSocketSession::SessionInfo( SessionPK sessionId, RequestId requestId )ι->void{
		LogRead( Ƒ("SessionInfo={:x}", sessionId), requestId );
		if( auto info = Web::Server::Sessions::Find( sessionId ); info ){
			LogWrite( Ƒ("SessionInfo userPK: {}, endpoint: {}, hasSocket: {}", info->UserPK.Value, info->UserEndpoint, info->HasSocket), requestId );
			Write( ToProto(move(*info), requestId) );
		}else
			WriteException( Exception{"Session not found."}, requestId );
	}
	α ServerSocketSession::SetSessionId( SessionPK sessionId, RequestId requestId )->Web::Server::Sessions::UpsertAwait::Task{
		try{
			LogRead( Ƒ("SetSessionId={:x}", sessionId), requestId );
			co_await Web::Server::Sessions::UpsertAwait( Ƒ("{:x}", sessionId), _userEndpoint.address().to_string(), true, nullptr );
			base::SetSessionId( sessionId );
			Write( FromServer::Complete(requestId) );
		}
		catch( IException& e ){
			WriteException( move(e), requestId );
		}
	}


	α ServerSocketSession::OnRead( Proto::FromClient::Transmission&& t )ι->void{
		ProcessTransmission( move(t), _userPK, nullopt );
	}

	α ServerSocketSession::GetJwt( Jde::RequestId requestId )ι->TAwait<jobject>::Task{
		try{
			THROW_IF( !_userPK, "Not logged in to system." );
			let user = co_await QL::QLAwait<jobject>( Ƒ("user(id:{}){{name target}}", _userPK->Value), {UserPK::System}, Server::Schemas() );
			let info = Web::Server::Sessions::Find( SessionId() );
			let expiration = Chrono::ToClock<Clock,steady_clock>( info->Expiration );
			Write( FromServer::Jwt(Server::GetJwt(*_userPK, string{user.at("name").as_string()}, string{user.at("target").as_string()}, _userEndpoint.address().to_string(), SessionId(), expiration, {}), requestId) );
		}
		catch( exception& e ){
			WriteException( move(e), requestId );
		}
	}

	α ServerSocketSession::ProcessTransmission( Proto::FromClient::Transmission&& transmission, optional<Jde::UserPK> /*userPK*/, optional<RequestId> clientRequestId )ι->void{
		uint cLog{}, cString{};
		if( transmission.messages_size()==0 )
			LogRead( "No messages in transmission.", 0, ELogLevel::Error );

		for( auto i=0; i<transmission.messages_size(); ++i ){
			auto& m = *transmission.mutable_messages( i );
			using enum Proto::FromClient::Message::ValueCase;
			let requestId = clientRequestId.value_or( m.request_id() );
			switch( m.Value_case() ){
			[[unlikely]]case kInstance:
				AddInstance( move(*m.mutable_instance()), requestId );
				break;
			case kAddSession:{
				AddSession( move(*m.mutable_add_session()), requestId, SRCE_CUR );
				break;}
			case kException:
				if( !requestId ){
					DBGT( ELogTags::SocketServerRead | ELogTags::Exception, "[{:x}.{:x}]Exception - {}", Id(), 0, m.exception().what() );
				}else if( !ForwardExecutionAwait::Resume( move(*m.mutable_execute_response()), requestId) )
					LogRead( Ƒ("Exception not handled - {}", m.exception().what()), requestId, ELogLevel::Critical );
				break;
			case kExecute:
			case kExecuteAnonymous:{
				bool isAnonymous = m.Value_case()==kExecuteAnonymous;
				auto bytes = isAnonymous ? move( *m.mutable_execute_anonymous() ) : move( *m.mutable_execute()->mutable_transmission() );
				optional<Jde::UserPK> executor = m.Value_case()==kExecuteAnonymous ? nullopt : optional<Jde::UserPK>( {m.execute().user_pk()} );
				LogRead( Ƒ("Execute{} size: {:10L}", isAnonymous ? "Anonymous" : "", bytes.size()), requestId );
				Execute( move(bytes), executor, requestId );
				break;}
			case kExecuteResponse:
				if( !ForwardExecutionAwait::Resume( move(*m.mutable_execute_response()), requestId) )
					LogRead( Ƒ("ExecuteResponse requestId:{} not found.", requestId), requestId, ELogLevel::Critical );
				break;
			case kForwardExecution:
			case kForwardExecutionAnonymous:{
				let anonymous = m.Value_case()==kForwardExecutionAnonymous;
				auto forward = anonymous ? m.mutable_forward_execution_anonymous() : m.mutable_forward_execution();
				ForwardExecution( move(*forward), anonymous, requestId );
				break;}
			[[likely]]case kQuery:{
				auto& query = *m.mutable_query();
				GraphQL( move(*query.mutable_text()), query.return_raw(), requestId );
				break;}
			[[likely]]case kLogEntry:
				++cLog;
				SaveLogEntry( move(*m.mutable_log_entry()), requestId );
				break;
			case kRequestType:
				if( m.request_type()==Proto::FromClient::ERequestType::Jwt )
					GetJwt( requestId );
				else
					WriteException( Exception{"RequestType: '{}' not implemented.", (uint32)m.request_type()}, requestId );
				break;
			case kSessionId:
				if( !m.session_id() )
					WriteException( Exception{"SessionId not set."}, requestId );
				else
					SetSessionId( m.session_id(), requestId );
			break;
			case kSessionInfo:
				SessionInfo( m.session_info(), requestId );
				break;
			[[likely]]case kStatus:{
				auto& status = *m.mutable_status();
				//:10L
				LogRead( "Status", requestId );
				Server::BroadcastStatus( _appPK, _instancePK, _instance.host(), move(status) );
				break;}
			case kSubscription:{
				auto& s = *m.mutable_subscription();
				LogRead( Ƒ("Subscription - {}", s.substr(0, MaxLogLength())), requestId );
				try{
					Write( FromServer::SubscriptionAck(AddSubscription(move(s), requestId), requestId) );
				}
				catch( std::exception& e ){
					WriteException( move(e), requestId );
				}
				break;}
			case kUnsubscription:{
				auto& v = *m.mutable_unsubscription();
				LogRead( Ƒ("Unsubscription - {}", v.request_ids().size()), requestId );
				vector<QL::SubscriptionId> subIds;
				for_each( v.request_ids(), [&]( auto id ){ subIds.emplace_back(id); } );
				RemoveSubscription( move(subIds), requestId );
				break;}
			[[likely]]case kStringField:{
/*				if( !_appPK || !_instancePK ){
					WriteException( Exception{"ApplicationId or InstanceId not set.", ELogLevel::Warning}, requestId );
					continue;
				}
				++cString;
				auto& s = *m.mutable_string_value();
				uuid id{ Jde::Proto::ToGuid(s.id()) };
				if( StringCache::Add( s.field(), id, s.value(), ELogTags::SocketServerRead) )
					Server::SaveString( (Proto::FromClient::EFields)s.field(), id, move(*s.mutable_value()) );*/
				break;}
			case kSubscribeLogs:{
				if( m.subscribe_logs().empty() ){
					LogRead( Ƒ("SubscribeLogs unsubscribe"), requestId );
					Server::UnsubscribeLogs( InstancePK() );
				}
				else{
					try{
						LogRead( Ƒ("SubscribeLogs subscribe - {}", m.subscribe_logs()), requestId );
						Server::SubscribeLogs( move(*m.mutable_subscribe_logs()), SharedFromThis() );
					}
					catch( IException& e ){
						WriteException( move(e), requestId );
					}
				}
				break;}
			case kSubscribeStatus:
				LogRead( Ƒ("SubscribeStatus - {}", m.subscribe_status()), requestId );
				if( m.subscribe_status() )
					Server::SubscribeStatus( *this );
				else
					Server::UnsubscribeStatus( InstancePK() );
				break;
			default:
				LogRead( Ƒ("Unknown message type '{}'", underlying(m.Value_case())), requestId, ELogLevel::Critical );
			}
		}
		if( cLog || cString )
			TRACET( ELogTags::SocketServerRead, "[{:x}] log entries recieved: {} strings received: {}.", Id(), cLog, cString );
	}

	α ServerSocketSession::OnClose()ι->void{
		LogRead( "OnClose", 0 );
		Server::RemoveSession( Id() );
		base::OnClose();
	}

	α ServerSocketSession::WriteComplete( RequestId requestId )ι->void{
		LogWrite( "Complete", requestId );
		Write( FromServer::Complete(requestId) );
	}
	α ServerSocketSession::WriteException( exception&& e, RequestId requestId )ι->void{
		LogWriteException( e, requestId );
		Write( FromServer::Exception(move(e), requestId) );
	}
	α ServerSocketSession::WriteException(std::string&& e, Jde::RequestId requestId)ι->void{
		LogWriteException( e, requestId );
		Write( FromServer::Exception(move(e), requestId) );
	}
	α ServerSocketSession::WriteSubscriptionAck( vector<QL::SubscriptionId>&& subscriptionIds, RequestId requestId )ι->void{
		Write( FromServer::SubscriptionAck(move(subscriptionIds), requestId) );
	}
	α ServerSocketSession::WriteSubscription( const jvalue& j, RequestId requestId )ι->void{
		auto serialized = serialize( j );
		LogWrite( Ƒ("Subscription: {}", serialized.substr(0,100)), requestId );
		Write( FromServer::Subscription(move(serialized), requestId) );
	}

	α ToProto( const Web::Server::SessionInfo& session, RequestId requestId )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		auto& m = *t.add_messages();
		m.set_request_id( requestId );
		auto& response = *m.mutable_session_info();
		*response.mutable_expiration() = Jde::Proto::ToTimestamp( Chrono::ToClock<Clock,steady_clock>(session.Expiration) );
		response.set_session_id( session.SessionId );
		response.set_user_pk( session.UserPK );
		response.set_user_endpoint( session.UserEndpoint );
		response.set_has_socket( session.HasSocket );
		return t;
	}
}
#include <jde/app/proto/app.FromServer.h>
#include <jde/fwk/io/protobuf.h>
#include <jde/ql/types/Subscription.h>
#include <jde/web/Jwt.h>
#include <jde/web/server/Web.FromServer.h>

#define let const auto

namespace Jde::App::FromServer{
	//AppClientSocketSession::ProcessTransmission used to PopTask for every kind ahead of its switch, and nine of them never
	//look at the handle - so a server-originated push (kClientQuery, kExecute, kSubscription, kTraces, ...) erased and
	//dropped the handle of whichever request of the client's happened to share that id.  The ids do collide: the client
	//allocates from a process-global counter and the server from a per-session one, both starting at 1, and
	//Server::QuerySessions fans a kClientQuery out to every app client.  The dropped coroutine is never resumed and never
	//failed, and the 60s watchdog cannot rescue it - AddTimeout starts with `if( !HasTask(requestId) ) co_return;` and
	//PopTask has already erased the entry - so the caller waits out the process.
	//Every enumerator is listed and there is no default, so adding a kind is a -Wswitch error rather than a silent
	//classification; when in doubt a kind is a push, which fails safe (an unanswered request still trips the watchdog).
	α IsResponse( Proto::FromServer::Message::ValueCase kind )ι->bool{
		using enum Proto::FromServer::Message::ValueCase;
		switch( kind ){
		case kConnectionInfo: case kException: case kGeneric: case kJwt: case kProgress:
		case kQueryResult: case kSessionInfo: case kStrings: case kSubscriptionAck:
			return true;
		case kAck: case kClientQuery: case kExecute: case kExecuteAnonymous: case kExecuteResponse:
		case kStringPks: case kSubscription: case kTraces: case VALUE_NOT_SET:
			return false;
		}
		return false;
	}

	Ω setMessage( RequestId requestId, function<void(Proto::FromServer::Message&)> set )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		auto& m = *t.add_messages();
		m.set_request_id( requestId );
		set( m );
		return t;
	}
}
namespace Jde::App{
	α FromServer::Ack( uint32 serverSocketId )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		t.add_messages()->set_ack( serverSocketId );
		return t;
	}

	α FromServer::Complete( RequestId requestId )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		t.add_messages()->set_request_id( requestId );
		return t;
	}
	α FromServer::ConnectionInfo( ProgramPK appPK, ProgInstPK instancePK, ConnectionPK connectionPK, RequestId clientRequestId, const Crypto::PublicKey& appServerPubKey, sp<Web::Server::SessionInfo> session, optional<bool> authResult )ι->Proto::FromServer::Transmission{
		return setMessage( clientRequestId, [&](auto& m){
			auto& info = *m.mutable_connection_info();
			info.set_app_pk( appPK );
			info.set_instance_pk( instancePK );
			info.set_connection_pk( connectionPK );
			info.set_certificate_modulus( {appServerPubKey.Modulus.begin(), appServerPubKey.Modulus.end()} );
			info.set_certificate_exponent( {appServerPubKey.Exponent.begin(), appServerPubKey.Exponent.end()} );
			if( authResult )
				info.set_auth_result( *authResult );
			*info.mutable_session_info() = move( Web::Server::ToProto(*session) );
			TRACET( ELogTags::Test, "Connected. UserPK='{}'", info.session_info().user_pk() );
		});
	}

	α FromServer::Exception( const runtime_error& e, optional<RequestId> requestId )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		auto& m = *t.add_messages();
		if( requestId )
			m.set_request_id( *requestId );
		auto& proto = *m.mutable_exception();
		proto.set_what( e.what() );
		if( let p = dynamic_cast<const Jde::Exception*>(&e); p ){
			proto.set_code( p->Code() );
			proto.set_status_code( p->HttpStatus() );//status & classification ride the base virtuals - the type can't cross the wire.
			proto.set_category( (Jde::Proto::ECategory)p->Category() );
			proto.set_category_code( p->CategoryCode() );
		}
		else
			proto.set_status_code( 500 );//plain std::exception - unclassified server fault.
		return t;
	}
	α FromServer::Exception( string&& e, optional<RequestId> requestId )ι->Proto::FromServer::Transmission{
		return setMessage( requestId.value_or(0), [&](auto& m){
			auto& proto = *m.mutable_exception();
			proto.set_what( move(e) );
		});
	}
	α FromServer::Jwt( Web::Jwt&& jwt, RequestId requestId )ι->Proto::FromServer::Transmission{
		return setMessage( requestId, [&](auto& m){
			m.set_jwt( jwt.Payload() );
		});
	}

	α FromServer::LogSubscription( ProgramPK appPK, ProgInstPK instancePK, const Logging::Entry& m, const QL::Subscription& sub )ι->Proto::FromServer::Transmission{
		return setMessage( sub.Id, [&](auto& msg){
			auto& traces = *msg.mutable_traces();
			traces.set_app_id( appPK );
			auto proto = traces.add_values();
			proto->set_id( sub.Id );
			proto->set_instance_id( instancePK );
			*proto->mutable_time() = Protobuf::ToTimestamp( m.Time );
			proto->set_level( (Log::Proto::ELogLevel)m.Level );
			proto->set_message_id( Protobuf::ToBytes(m.Id()) );
			proto->set_file_id( Protobuf::ToBytes(m.FileId()) );
			proto->set_function_id( Protobuf::ToBytes(m.FunctionId()) );
			proto->set_line( m.Line );
			proto->set_user_pk( m.UserPK.Value );
			for( let& arg : m.Arguments )
				proto->add_args( arg );
		});
	}

	α FromServer::QueryClient( string&& query, sp<jobject> variables, Jde::UserPK executer, bool raw, RequestId requestId )ι->Proto::FromServer::Transmission{
		return setMessage( requestId, [&](auto& m){
			auto& clientQuery = *m.mutable_client_query();
			clientQuery.set_query( move(query) );
			clientQuery.set_executer_pk( executer.Value );//.Value: UserPK converts to bool implicitly, so the raw struct sets 1 for every non-zero user.
			clientQuery.set_raw( raw );
			if( variables )
				*clientQuery.mutable_variables() = serialize(*variables);
		});
	}
/*
	α FromServer::ToStatus( ProgramPK appId, ProgInstPK instanceId, str hostName, Proto::FromClient::Status&& input )ι->Proto::FromServer::Status{
		Proto::FromServer::Status output;
		output.set_application_id( (google::protobuf::uint32)appId );
		output.set_instance_id( (google::protobuf::uint32)instanceId );
		output.set_host_name( hostName );
		*output.mutable_start_time() = input.start_time();
		output.set_memory( input.memory() );
		*output.mutable_values() = move( *input.mutable_values() );
		return output;
	}

	α FromServer::StatusBroadcast( Proto::FromServer::Status status )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		*t.add_messages()->mutable_status() = move( status );
		return t;
	}
*/
	α FromServer::SubscriptionAck( flat_set<QL::SubscriptionId>&& subscriptionIds, RequestId requestId )ι->Proto::FromServer::Transmission{
		return setMessage( requestId, [&](auto& m){
			auto& ack = *m.mutable_subscription_ack();
			for_each( subscriptionIds, [&](auto id){ack.add_server_ids(id);} );
		});
	}
	α FromServer::Subscription( string&& s, RequestId requestId )ι->Proto::FromServer::Transmission{
		return setMessage( requestId, [&](auto& m){
			*m.mutable_subscription() = move( s );
		});
	}

	α FromServer::ExecuteRequest( RequestId serverRequestId, UserPK userPK, string&& fromClient )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		auto toServer = t.add_messages();
		toServer->set_request_id( serverRequestId );
		if( userPK ){
			auto customExecute = toServer->mutable_execute();
			customExecute->set_user_pk( userPK.Value );//.Value: UserPK converts to bool implicitly, so the raw struct sets 1 for every non-zero user.
			*customExecute->mutable_transmission() = move( fromClient );
		}
		else
			toServer->set_execute_anonymous( move(fromClient) );
		return t;
	}

	α FromServer::Execute( string&& executionResult, RequestId clientRequestId )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		auto toServer = t.add_messages();
		toServer->set_request_id( clientRequestId );
		toServer->set_execute_response( move(executionResult) );
		return t;
	}

	α FromServer::GraphQL( string&& queryResults, RequestId requestId )ι->Proto::FromServer::Transmission{
		return setMessage( requestId, [&](auto& m){
			m.set_query_result( move(queryResults) );
		});
	}
	α FromServer::Session( const Web::Server::SessionInfo& session, RequestId requestId )->Proto::FromServer::Transmission{
		return setMessage( requestId, [&](auto& m){
			*m.mutable_session_info() = Web::Server::ToProto( session );
		});
	}
}
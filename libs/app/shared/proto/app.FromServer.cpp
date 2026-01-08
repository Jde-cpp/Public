#include <jde/app/proto/app.FromServer.h>
#include <jde/fwk/io/protobuf.h>
#include <jde/db/Row.h>
#include <jde/db/meta/Table.h>
#include <jde/db/meta/Column.h>
#include <jde/ql/types/Subscription.h>
#include <jde/ql/types/TableQL.h>
#include <jde/web/Jwt.h>
#include <jde/web/server/Web.FromServer.h>

#define let const auto

namespace Jde::App::FromServer{
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
	α FromServer::ConnectionInfo( ProgramPK appPK, ProgInstPK instancePK, ConnectionPK connectionPK, RequestId clientRequestId, const Crypto::PublicKey& appServerPubKey, Web::Server::SessionInfo&& session )ι->Proto::FromServer::Transmission{
		return setMessage( clientRequestId, [&](auto& m){
			auto& info = *m.mutable_connection_info();
			info.set_app_pk( appPK );
			info.set_instance_pk( instancePK );
			info.set_connection_pk( connectionPK );
			info.set_certificate_modulus( {appServerPubKey.Modulus.begin(), appServerPubKey.Modulus.end()} );
			info.set_certificate_exponent( {appServerPubKey.Exponent.begin(), appServerPubKey.Exponent.end()} );
			*info.mutable_session_info() = move( Web::Server::ToProto(move(session)) );
		});
	}

	α FromServer::Exception( const exception& e, optional<RequestId> requestId )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		auto& m = *t.add_messages();
		if( requestId )
			m.set_request_id( *requestId );
		auto& proto = *m.mutable_exception();
		proto.set_what( e.what() );
		if( let p = dynamic_cast<const IException*>(&e); p )
			proto.set_code( p->Code );
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
			clientQuery.set_executer_pk( executer );
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
			customExecute->set_user_pk( userPK );
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
	α FromServer::ToTrace( DB::Row&& row, const vector<QL::ColumnQL>& columns )ι->Proto::FromServer::Trace{
		Proto::FromServer::Trace t;
		uint i=0;
		for( auto&& c : columns ){
			if( !c.DBColumn )
				continue;

			str name = c.DBColumn->Name;
			if( name=="id" )
				t.set_id( row.GetUInt32(i) );
			else if( name=="instance_id" )
				t.set_instance_id( row.GetUInt32(i) );
			else if( name=="file_id" )
				t.set_file_id( Protobuf::ToBytes(row.GetGuid(i)) );
			else if( name=="function_id" )
				t.set_function_id( Protobuf::ToBytes(row.GetGuid(i)) );
			else if( name=="line_number" )
				t.set_line( row.GetUInt32(i) );
			else if( name=="message_id" )
				t.set_message_id( Protobuf::ToBytes(row.GetGuid(i)) );
			else if( name=="level" )
				t.set_level( (Log::Proto::ELogLevel)row.GetUInt16(i) );
			else if( name=="thread_id" )
				t.set_thread_id( row.GetUInt32(i) );
			else if( name=="time" )
				*t.mutable_time() = Protobuf::ToTimestamp( row.GetTimePoint(i) );
			else if( name=="user_pk" )
				t.set_user_pk( row.GetUInt32(i) );
			else{
				BREAK;
			}
			++i;
		}
		return t;
	}
}
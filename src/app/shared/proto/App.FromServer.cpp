#include <jde/app/shared/proto/App.FromServer.h>
#include <jde/web/server/Sessions.h>
#include "../../../../Framework/source/db/Row.h"
#include "../../../../Framework/source/db/GraphQL.h"
#include "../../../../Framework/source/db/types/Table.h"
#include "../../../../Framework/source/io/ProtoUtilities.h"

#define var const auto

namespace Jde::App{
	α FromServer::Ack( uint serverSocketId )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		t.add_messages()->set_ack( serverSocketId );
		return t;
	}

	α FromServer::Complete( RequestId requestId )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		t.add_messages()->set_request_id( requestId );
		return t;
	}

	α FromServer::SessionInfo( const Web::Server::SessionInfo& session, RequestId requestId )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		auto& m = *t.add_messages();
		m.set_request_id( requestId );
		auto& response = *m.mutable_session_info();
		*response.mutable_expiration() = IO::Proto::ToTimestamp( Chrono::ToClock<Clock,steady_clock>(session.Expiration) );
		response.set_session_id( session.SessionId );
		response.set_user_pk( session.UserPK );
		response.set_user_endpoint( session.UserEndpoint );
		response.set_has_socket( session.HasSocket );
		return t;
	}

	α FromServer::Exception( const IException& e, optional<RequestId> requestId )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		auto& m = *t.add_messages();
		if( requestId )
			m.set_request_id( *requestId );
		auto& proto = *m.mutable_exception();
		proto.set_what( e.what() );
		proto.set_code( e.Code );
		return t;
	}

	α FromServer::ToStatus( AppPK appId, AppInstancePK instanceId, str hostName, Proto::FromClient::Status&& input )ι->Proto::FromServer::Status{
		Proto::FromServer::Status output;
		output.set_application_id( (google::protobuf::uint32)appId );
		output.set_instance_id( (google::protobuf::uint32)instanceId );
		output.set_host_name( hostName );
		*output.mutable_start_time() = input.start_time();
		output.set_db_log_level( input.server_min_log_level() );
		output.set_file_log_level( input.client_min_log_level() );
		output.set_memory( input.memory() );
		*output.mutable_values() = move( *input.mutable_details() );
		return output;
	}

	α FromServer::StatusBroadcast( Proto::FromServer::Status status )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		*t.add_messages()->mutable_status() = move( status );
		return t;
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
		Proto::FromServer::Transmission t;
		auto toServer = t.add_messages();
		toServer->set_request_id( requestId );
		toServer->set_graph_ql( move(queryResults) );
		return t;
	}
	α FromServer::ToTrace( const DB::IRow& row, const vector<DB::ColumnQL>& columns )ι->Proto::FromServer::Trace{
		Proto::FromServer::Trace t;
		uint i=0;
		for( auto&& c : columns ){
			if( !c.SchemaColumnPtr )
				continue;

			str name = c.SchemaColumnPtr->Name;
			if( name=="id" )
				t.set_id( row.GetUInt32(i) );
			else if( name=="instance_id" )
				t.set_instance_id( row.GetUInt32(i) );
			else if( name=="file_id" )
				t.set_file_id( row.GetUInt32(i) );
			else if( name=="function_id" )
				t.set_function_id( row.GetUInt32(i) );
			else if( name=="line_number" )
				t.set_line( row.GetUInt32(i) );
			else if( name=="message_id" )
				t.set_message_id( row.GetUInt32(i) );
			else if( name=="level" )
				t.set_level( (Jde::Proto::ELogLevel)row.GetUInt16(i) );
			else if( name=="thread_id" )
				t.set_thread_id( row.GetUInt32(i) );
			else if( name=="time" )
				*t.mutable_time() = IO::Proto::ToTimestamp( row.GetTimePoint(i) );
			else if( name=="user_pk" )
				t.set_user_pk( row.GetUInt32(i) );
			else
				BREAK;
			++i;
		}
		return t;
	}
	α FromServer::TraceBroadcast( LogPK id, AppPK appId, AppInstancePK instanceId, const Logging::ExternalMessage& m, const vector<string>& args )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		auto traces = t.add_messages()->mutable_traces();
		traces->set_app_id( appId );
		auto proto = traces->add_values();
		proto->set_id( id );
		proto->set_instance_id( instanceId );
		*proto->mutable_time() = IO::Proto::ToTimestamp( m.TimePoint );
		proto->set_level( (Jde::Proto::ELogLevel)m.Level );
		proto->set_message_id( m.MessageId );
		proto->set_file_id( m.FileId );
		proto->set_function_id( m.FunctionId );
		proto->set_line( m.LineNumber );
		proto->set_user_pk( m.UserPK );
		proto->set_thread_id( m.ThreadId );
		for( var& arg : args )
			proto->add_args( arg );
		return t;
	}
}
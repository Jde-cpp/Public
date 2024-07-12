#include <jde/appClient/proto/App.FromServer.h>
#define var const auto

namespace Jde::App{
	α FromServer::ExecuteResponseTransmission( RequestId clientRequestId, string&& executionResult )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		auto toServer = t.add_messages();
		toServer->set_request_id( clientRequestId );
		toServer->set_execute_response( move(executionResult) );
		return t;
	}
	α FromServer::Status( AppPK appId, AppInstancePK instanceId, str hostName, Proto::FromClient::Status&& input )ι->Proto::FromServer::Status{
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

	α FromServer::TraceTransmission( LogPK id, AppPK appId, AppInstancePK instanceId, const Proto::FromClient::LogEntry& m, const vector<string>& args )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		auto traces = t.add_messages()->mutable_traces();
		traces->set_app_id( appId );
		auto proto = traces->add_values();
		proto->set_id( id );
		proto->set_instance_id( instanceId );
		*proto->mutable_time() = m.time();
		proto->set_level( (Jde::Proto::ELogLevel)m.level() );
		proto->set_message_id( m.message_id() );
		proto->set_file_id( m.file_id() );
		proto->set_function_id( m.function_id() );
		proto->set_line_number( m.line() );
		proto->set_user_pk( m.user_pk() );
		proto->set_thread_id( m.thread_id() );
		for( var& arg : args )
			proto->add_variables( arg );
		return t;
	}
}
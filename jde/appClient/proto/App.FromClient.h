#pragma once
#include "../usings.h"
namespace Jde::App::FromClient{
	namespace PFromClient = Jde::App::Proto::FromClient;

	Ξ StatusMessage( vector<string>&& details )ι->PFromClient::Transmission{
		PFromClient::Transmission t;
		auto status = t.add_messages()->mutable_status();
		*status->mutable_start_time() = IO::Proto::ToTimestamp( Logging::StartTime() );
		status->set_memory( IApplication::MemorySize() );
		status->set_server_min_log_level( (Jde::Proto::ELogLevel)Logging::External::MinLevel("db") );
		status->set_client_min_log_level( (Jde::Proto::ELogLevel)Logging::ClientMinLevel() );
		for_each( details, [status](auto&& detail){ status->add_details(move(detail)); } );
		return t;
	}
	Ξ ConnectMessage( SessionPK sessionId, RequestId requestId )ι->PFromClient::Transmission{
		PFromClient::Transmission t;
		auto req = t.add_messages();
		req->set_session_id( sessionId );
		req->set_request_id( requestId );
		return t;
	}
	Ξ SessionInfoMessage( SessionPK sessionId, RequestId requestId )ι->PFromClient::Transmission{
		PFromClient::Transmission t;
		auto req = t.add_messages();
		req->set_session_info( sessionId );
		req->set_request_id( requestId );
		return t;
	}
	/**/
/*	Ξ ToExternalLogEntry( PFromClient::LogEntry&& proto )ι->Logging::ExternalMessage{
		Logging::ExternalMessage m{ Logging::MessageBase{ (ELogLevel)proto.level(), proto.message_id(), proto.file_id(), proto.function_id(), proto.line(), proto.user_pk(), proto.thread_id()}, {}, IO::Proto::ToTimePoint( proto.time() ) };
		m.TimePoint = ;
		return m;
	}*/

	Ξ ToLogEntry( Logging::ExternalMessage m )ι->PFromClient::LogEntry{
		PFromClient::LogEntry proto;
		proto.set_level( (Jde::Proto::ELogLevel)m.Level );
		proto.set_message_id( m.MessageId );
		proto.set_file_id( m.FileId );
		proto.set_function_id( m.FunctionId );
		proto.set_line( m.LineNumber );
		proto.set_thread_id( m.ThreadId );
		proto.set_user_pk( m.UserPK );
		*proto.mutable_time() = IO::Proto::ToTimestamp( m.TimePoint );
		return proto;
	}
}
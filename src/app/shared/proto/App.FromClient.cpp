#include <jde/app/shared/proto/App.FromClient.h>
#include "../../../../Framework/source/io/ProtoUtilities.h"

namespace Jde::App{
	α FromClient::AddStringField( PFromClient::Transmission& t, PFromClient::EFields field, uint32 id, str value )ι->void{
		auto& m = *t.add_messages()->mutable_string_value();
		m.set_field( field );
		m.set_id( id );
		m.set_value( value );
	}
	α FromClient::Exception( IException&& e )ι->PFromClient::Transmission{
		PFromClient::Transmission t;
		auto& proto = *t.add_messages()->mutable_exception();
		proto.set_what( e.what() );
		proto.set_code( e.Code );
		return t;
	}

	α FromClient::GraphQL( str query, RequestId requestId )ι->PFromClient::Transmission{
		PFromClient::Transmission t;
		auto& m = *t.add_messages();
		m.set_request_id( requestId );
		*m.mutable_graph_ql() = query;
		return t;
	}

	α FromClient::ToStatus( vector<string>&& details )ι->PFromClient::Status{
		PFromClient::Status y;
		*y.mutable_start_time() = IO::Proto::ToTimestamp( Logging::StartTime() );
		y.set_server_min_log_level( (Jde::Proto::ELogLevel)Logging::External::MinLevel("db") );
		y.set_client_min_log_level( (Jde::Proto::ELogLevel)Logging::ClientMinLevel() );
		y.set_memory( IApplication::MemorySize() );
		for_each( details, [&y](auto&& detail){ y.add_details(move(detail)); } );
		return y;
	}

	α FromClient::Status( vector<string>&& details )ι->PFromClient::Transmission{
		PFromClient::Transmission t;
		auto& status = *t.add_messages()->mutable_status() = ToStatus( move(details) );
		*status.mutable_start_time() = IO::Proto::ToTimestamp( Logging::StartTime() );
		status.set_memory( IApplication::MemorySize() );
		status.set_server_min_log_level( (Jde::Proto::ELogLevel)Logging::External::MinLevel("db") );
		status.set_client_min_log_level( (Jde::Proto::ELogLevel)Logging::ClientMinLevel() );
		for_each( details, [&status](auto&& detail){ status.add_details(move(detail)); } );
		return t;
	}
/*
	α FromClient::ConnectTransmission( SessionPK sessionId, RequestId requestId )ι->PFromClient::Transmission{
		PFromClient::Transmission t;
		auto req = t.add_messages();
		req->set_connect_session_id( sessionId );
		req->set_request_id( requestId );
		return t;
	}
*/
	α FromClient::Session( SessionPK sessionId, RequestId requestId )ι->PFromClient::Transmission{
		PFromClient::Transmission t;
		auto req = t.add_messages();
		req->set_session_info( sessionId );
		req->set_request_id( requestId );
		return t;
	}
	/**/
/*	α ToExternalLogEntry( PFromClient::LogEntry&& proto )ι->Logging::ExternalMessage{
		Logging::ExternalMessage m{ Logging::MessageBase{ (ELogLevel)proto.level(), proto.message_id(), proto.file_id(), proto.function_id(), proto.line(), proto.user_pk(), proto.thread_id()}, {}, IO::Proto::ToTimePoint( proto.time() ) };
		m.TimePoint = ;
		return m;
	}
*/
	α FromClient::ToLogEntry( Logging::ExternalMessage m )ι->PFromClient::LogEntry{
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
//	α ToExternalMessage( const PFromClient::LogEntry& m )ι->Logging::ExternalMessage{
//		return message;
//	}
}
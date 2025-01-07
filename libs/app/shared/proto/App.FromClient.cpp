#include <jde/app/shared/proto/App.FromClient.h>
#include <jde/framework/io/proto.h>

namespace Jde::App::FromClient{
	Ω setMessage( RequestId requestId, function<void(PFromClient::Message&)> f ){
		PFromClient::Transmission t;
		auto& m = *t.add_messages();
		m.set_request_id( requestId );
		f( m );
		return t;
	}
}
namespace Jde::App{
	α FromClient::AddSession( str domain, str loginName, Access::ProviderPK providerPK, str userEndPoint, bool isSocket, RequestId requestId )ι->PFromClient::Transmission{
		PFromClient::Transmission t;
		auto& m = *t.add_messages();
		m.set_request_id( requestId );
		auto& s = *m.mutable_add_session();
		s.set_domain( domain );
		s.set_login_name( loginName );
		s.set_provider_pk( providerPK );
		s.set_user_endpoint( userEndPoint );
		s.set_is_socket( isSocket );
		return t;
	}

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
		proto.set_code( (uint32)e.Code );
		return t;
	}

	α FromClient::Instance( str application, str instanceName, SessionPK sessionId, RequestId requestId )ι->PFromClient::Transmission{
		PFromClient::Transmission t;
		auto& m = *t.add_messages();
		m.set_request_id( requestId );
		auto& i = *m.mutable_instance();
		i.set_application( application );
		i.set_instance_name( instanceName );
		i.set_session_id( sessionId );
		i.set_host( IApplication::HostName() );
		i.set_pid( OSApp::ProcessId() );
		i.set_server_log_level( (Jde::Proto::ELogLevel)Logging::External::MinLevel("db") );
		i.set_client_log_level( (Jde::Proto::ELogLevel)Logging::ClientMinLevel() );
		*i.mutable_start_time() = Jde::Proto::ToTimestamp( Logging::StartTime() );
		i.set_web_port( Settings::FindNumber<PortType>("http/port").value_or(0) );

		return t;
	}
	α FromClient::Query( string query, RequestId requestId )ι->PFromClient::Transmission{
		return setMessage( requestId, [&](auto& m){ *m.mutable_query()=move(query); } );
	}

	α FromClient::ToStatus( vector<string>&& details )ι->PFromClient::Status{
		PFromClient::Status y;
		*y.mutable_start_time() = Jde::Proto::ToTimestamp( Logging::StartTime() );
		y.set_server_min_log_level( (Jde::Proto::ELogLevel)Logging::External::MinLevel("db") );
		y.set_client_min_log_level( (Jde::Proto::ELogLevel)Logging::ClientMinLevel() );
		y.set_memory( IApplication::MemorySize() );
		for_each( details, [&y](auto&& detail){ y.add_details(move(detail)); } );
		return y;
	}

	α FromClient::Status( vector<string>&& details )ι->PFromClient::Transmission{
		PFromClient::Transmission t;
		auto& status = *t.add_messages()->mutable_status() = ToStatus( move(details) );
		*status.mutable_start_time() = Jde::Proto::ToTimestamp( Logging::StartTime() );
		status.set_memory( IApplication::MemorySize() );
		status.set_server_min_log_level( (Jde::Proto::ELogLevel)Logging::External::MinLevel("db") );
		status.set_client_min_log_level( (Jde::Proto::ELogLevel)Logging::ClientMinLevel() );
		for_each( details, [&status](auto&& detail){ status.add_details(move(detail)); } );
		return t;
	}

	α FromClient::Session( SessionPK sessionId, RequestId requestId )ι->PFromClient::Transmission{
		PFromClient::Transmission t;
		auto req = t.add_messages();
		req->set_session_info( sessionId );
		req->set_request_id( requestId );
		return t;
	}

	α FromClient::Subscription( string&& query, RequestId requestId )ι->PFromClient::Transmission{
		return setMessage( requestId, [&](auto& m){
			*m.mutable_subscription() = move( query );
		});
	}

	α FromClient::ToLogEntry( Logging::ExternalMessage m )ι->PFromClient::LogEntry{
		PFromClient::LogEntry proto;
		proto.set_level( (Jde::Proto::ELogLevel)m.Level );
		proto.set_message_id( m.MessageId );
		proto.set_file_id( m.FileId );
		proto.set_function_id( m.FunctionId );
		proto.set_line( m.LineNumber );
		proto.set_thread_id( m.ThreadId );
		proto.set_user_pk( m.UserPK.Value );
		*proto.mutable_time() = Jde::Proto::ToTimestamp( m.TimePoint );
		return proto;
	}
}
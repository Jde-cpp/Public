#include <jde/app/shared/proto/App.FromClient.h>
#include <jde/framework/io/proto.h>
#include <jde/framework/settings.h>

#include <boost/uuid/uuid_io.hpp>

namespace Jde::App::FromClient{
	Ω setMessage( RequestId requestId, function<void(PFromClient::Message&)> f ){
		PFromClient::Transmission t;
		auto& m = *t.add_messages();
		m.set_request_id( requestId );
		f( m );
		return t;
	}
	Ω transString( RequestId requestId, function<void(PFromClient::Message&)> f ){
		auto t = setMessage( requestId, f );
		return Jde::Proto::ToString( t );
	}
}
namespace Jde::App{
	α FromClient::AddSession( str domain, str loginName, Access::ProviderPK providerPK, str userEndPoint, bool isSocket, RequestId requestId )ι->StringTrans{
		return transString( requestId, [&](auto& m){
			auto& s = *m.mutable_add_session();
			s.set_domain( domain );
			s.set_login_name( loginName );
			s.set_provider_pk( providerPK );
			s.set_user_endpoint( userEndPoint );
			s.set_is_socket( isSocket );
		} );
	}
	α FromClient::AddStringField( PFromClient::Transmission& t, PFromClient::EFields field, uuid id, str value )ι->void{
		auto& m = *t.add_messages()->mutable_string_value();
		m.set_field( field );
		m.set_id( Jde::Proto::ToBytes(id) );
		m.set_value( value );
	}
	α FromClient::Exception( exception&& e, RequestId requestId )ι->PFromClient::Transmission{
		return setMessage( requestId, [&](auto& m){
			auto& request = *m.mutable_exception();
			request.set_what( e.what() );
			if( auto p = dynamic_cast<IException*>(&e); p )
				request.set_code( (uint32)p->Code );
		} );
	}
	α FromClient::Exception( string&& e, RequestId requestId )ι->PFromClient::Transmission{
		return setMessage( requestId, [&](auto& m){
			auto& request = *m.mutable_exception();
			request.set_what( move(e) );
		} );
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
		*i.mutable_start_time() = Jde::Proto::ToTimestamp( Process::StartTime() );
		i.set_web_port( Settings::FindNumber<PortType>("/http/port").value_or(0) );

		return t;
	}
	α FromClient::Query( string query, RequestId requestId, bool returnRaw )ι->PFromClient::Transmission{
		return setMessage( requestId, [&](auto& m){
			auto& request = *m.mutable_query();
			request.set_text( query );
			request.set_return_raw( returnRaw );
		} );
	}
	α FromClient::Jwt( RequestId requestId )ι->StringTrans{
		return transString( requestId, [&](auto& m){ m.set_request_type( Proto::FromClient::ERequestType::Jwt );} );
	}

	α FromClient::ToStatus( vector<string>&& details )ι->PFromClient::Status{
		PFromClient::Status y;
		*y.mutable_start_time() = Jde::Proto::ToTimestamp( Process::StartTime() );
		y.set_memory( IApplication::MemorySize() );
		for_each( details, [&y](auto&& detail){ y.add_details(move(detail)); } );
		return y;
	}

	α FromClient::Status( vector<string>&& details )ι->PFromClient::Transmission{
		PFromClient::Transmission t;
		auto& status = *t.add_messages()->mutable_status() = ToStatus( move(details) );
		*status.mutable_start_time() = Jde::Proto::ToTimestamp( Process::StartTime() );
		status.set_memory( IApplication::MemorySize() );
		for_each( details, [&status](auto&& detail){ status.add_details(move(detail)); } );
		return t;
	}

	α FromClient::Session( SessionPK sessionId, RequestId requestId )ι->StringTrans{
		return transString( requestId, [&](auto& m){
			m.set_session_info( sessionId );
		});
	}

	α FromClient::Subscription( string&& query, RequestId requestId )ι->PFromClient::Transmission{
		return setMessage( requestId, [&](auto& m){
			*m.mutable_subscription() = move( query );
		});
	}

	α FromClient::ToLogEntry( Logging::Entry m )ι->PFromClient::LogEntry{
		PFromClient::LogEntry proto;
		proto.set_level( (Jde::Proto::ELogLevel)m.Level );
		proto.set_message_id( Jde::Proto::ToBytes(m.Id()) );
		proto.set_file_id( Jde::Proto::ToBytes(m.FileId()) );
		proto.set_function_id( Jde::Proto::ToBytes(m.FunctionId()) );
		proto.set_line( m.Line );
		proto.set_user_pk( m.UserPK.Value );
		*proto.mutable_time() = Jde::Proto::ToTimestamp( m.Time );
		return proto;
	}
	α FromClient::FromLogEntry( PFromClient::LogEntry&& m )ι->Logging::Entry{
		return Logging::Entry{
			(ELogLevel)m.level(),
			(ELogTags)m.tags(),
			m.line(),
			Jde::Proto::ToTimePoint( m.time() ),
			{m.user_pk()},
			Jde::Proto::ToGuid(m.message_id()),
			Jde::Proto::ToGuid(m.file_id()),
			Jde::Proto::ToGuid(m.function_id()),
			Jde::Proto::ToVector( move(*m.mutable_args()) )
		};
	}
}
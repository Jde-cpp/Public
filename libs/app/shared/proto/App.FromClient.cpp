#include <jde/app/shared/proto/App.FromClient.h>
#include <boost/uuid/uuid_io.hpp>
#include <jde/fwk/io/proto.h>
#include <jde/fwk/settings.h>
#include "Log.pb.h"

using Jde::Proto::ToBytes;
namespace Jde::App::FromClient{
	Ω setMessage( RequestId requestId, function<void(PFromClient::Message&)> f )ι->PFromClient::Transmission{
		PFromClient::Transmission t;
		auto& m = *t.add_messages();
		m.set_request_id( requestId );
		f( m );
		return t;
	}
	Ω transString( RequestId requestId, function<void(PFromClient::Message&)> f )ι->string{
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
	α FromClient::ToString( uuid id, string&& value )ι->Log::Proto::String{
		Log::Proto::String m;
		*m.mutable_id() = ToBytes( id );
		*m.mutable_value() = move( value );
		return m;
	}
/*	α FromClient::AddStringField( PFromClient::Transmission& t, Log::Proto::EFields field, uuid id, string&& value )ι->void{
		auto& m = *t.add_messages()->mutable_string_field();
		m.set_field( field );
		*m.mutable_value() = ToString( id, move(value) );
	}
*/
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
		i.set_host( Process::HostName() );
		i.set_pid( Process::ProcessId() );
		*i.mutable_start_time() = Jde::Proto::ToTimestamp( Process::StartTime() );
		i.set_web_port( Settings::FindNumber<PortType>("/http/port").value_or(0) );

		return t;
	}
	α FromClient::Query( string query, jobject variables, RequestId requestId, bool returnRaw )ι->string{
		return transString( requestId, [&](auto& m){
			auto& request = *m.mutable_query();
			request.set_text( query );
			request.set_return_raw( returnRaw );
			*request.mutable_variables() = serialize( move(variables) );
		} );
	}

	α FromClient::Jwt( RequestId requestId )ι->StringTrans{
		return transString( requestId, [&](auto& m){ m.set_request_type( Proto::FromClient::ERequestType::Jwt );} );
	}

	α FromClient::ToStatus( vector<string>&& details )ι->PFromClient::Status{
		PFromClient::Status y;
		*y.mutable_start_time() = Jde::Proto::ToTimestamp( Process::StartTime() );
		y.set_memory( Process::MemorySize() );
		for_each( details, [&y](auto&& detail){ y.add_details(move(detail)); } );
		return y;
	}

	α FromClient::Status( vector<string>&& details )ι->PFromClient::Transmission{
		PFromClient::Transmission t;
		auto& status = *t.add_messages()->mutable_status() = ToStatus( move(details) );
		*status.mutable_start_time() = Jde::Proto::ToTimestamp( Process::StartTime() );
		status.set_memory( Process::MemorySize() );
		for_each( details, [&status](auto&& detail){ status.add_details(move(detail)); } );
		return t;
	}

	α FromClient::Session( SessionPK sessionId, RequestId requestId )ι->StringTrans{
		return transString( requestId, [&](auto& m){
			m.set_session_info( sessionId );
		});
	}

	α FromClient::Subscription( string&& query, jobject variables, RequestId requestId )ι->string{
		return transString( requestId, [&](auto& m){
			auto& sub = *m.mutable_subscription();
			sub.set_text( move(query) );
			*sub.mutable_variables() = serialize(move(variables));
		});
	}
	α FromClient::LogEntries( vector<Logging::Entry>&& entries )ι->PFromClient::Transmission{
		PFromClient::Transmission t;
		for( auto&& entry : entries ){
			*t.add_messages()->mutable_log_entry() = LogEntryClient( move(entry) );
		}
		return t;
	}

	α FromClient::LogEntryClient( Logging::Entry&& e )ι->Log::Proto::LogEntryClient{
		Log::Proto::LogEntryClient proto;
		proto.set_text( move(e.Text) );
		*proto.mutable_args() = Jde::Proto::FromVector( move(e.Arguments) );
		proto.set_level( (Log::Proto::ELogLevel)e.Level );
		proto.set_tags( (uint)e.Tags );
		proto.set_line( e.Line );
		*proto.mutable_time() = Jde::Proto::ToTimestamp( e.Time );
		proto.set_user_pk( e.UserPK.Value );
		proto.set_file( move(e.FileString()) );
		proto.set_function( move(e.FunctionString()) );

		return proto;
	}
	α FromClient::LogEntryFile( const Logging::Entry& m )ι->Log::Proto::LogEntryFile{
		Log::Proto::LogEntryFile proto;
		proto.set_template_id( ToBytes(m.Id()) );
		for( auto& arg : m.Arguments )
			*proto.add_args() = ToBytes( Logging::Entry::GenerateId(arg) );
		proto.set_level( (Log::Proto::ELogLevel)m.Level );
		proto.set_tags( (uint)m.Tags );
		proto.set_line( m.Line );
		*proto.mutable_time() = Jde::Proto::ToTimestamp( m.Time );
		proto.set_user_pk( m.UserPK.Value );
		proto.set_file_id( ToBytes(m.FileId()) );
		proto.set_function_id( ToBytes(m.FunctionId()) );
		return proto;
	}

	α FromClient::FromLogEntry( Log::Proto::LogEntryClient&& m )ι->Logging::Entry{
		return Logging::Entry{
			(ELogLevel)m.level(),
			(ELogTags)m.tags(),
			m.line(),
			Jde::Proto::ToTimePoint( m.time() ),
			{m.user_pk()},
			move(*m.mutable_text()),
			move(*m.mutable_file()),
			move(*m.mutable_function()),
			Jde::Proto::ToVector( move(*m.mutable_args()) )
		};
	}
}
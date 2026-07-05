#include "Log.pb.h"
#include "jde/fwk/usings.h"
#include "jde/fwk/chrono.h"
#include <jde/app/proto/LogProto.h>
#include <jde/fwk/io/protobuf.h>

#define let const auto
namespace Jde::App{
	using Jde::Protobuf::ToBytes;

	α LogProto::FromLogEntry( Log::Proto::LogEntryClient&& m )ι->Logging::Entry{
		return Logging::Entry{
			( ELogLevel )m.level(),
			( ELogTags )m.tags(),
			m.line(),
			Protobuf::ToTimePoint( m.time() ),
			{ m.user_pk() },
			move( *m.mutable_text() ),
			move( *m.mutable_file() ),
			move( *m.mutable_function() ),
			Protobuf::ToVector( move(*m.mutable_args()) )
		};
	}

	α LogProto::LogEntryClient( Logging::Entry&& e )ι->Log::Proto::LogEntryClient{
		Log::Proto::LogEntryClient proto;
		proto.set_text( move(e.Text) );
		*proto.mutable_args() = Protobuf::FromVector( move(e.Arguments) );
		proto.set_level( (Log::Proto::ELogLevel)e.Level );
		proto.set_tags( (uint)e.Tags );
		proto.set_line( e.Line );
		*proto.mutable_time() = Protobuf::ToTimestamp( e.Time );
		proto.set_user_pk( e.UserPK.Value );
		proto.set_file( move(e.FileString()) );
		proto.set_function( move(e.FunctionString()) );

		return proto;
	}

	Ŧ logEntry( const Logging::Entry& m, T& proto )ι->void{
		proto.set_template_id( ToBytes(m.Id()) );
		for( auto& arg : m.Arguments )
			*proto.add_args() = ToBytes( Logging::Entry::GenerateId(arg) );
		proto.set_level( (Log::Proto::ELogLevel)m.Level );
		proto.set_tags( (uint)m.Tags );
		proto.set_line( m.Line );
		*proto.mutable_time() = Protobuf::ToTimestamp( m.Time );
		proto.set_user_pk( m.UserPK.Value );
		proto.set_file_id( ToBytes(m.FileId()) );
		proto.set_function_id( ToBytes(m.FunctionId()) );
	}

	α LogProto::LogEntryFile( const Logging::Entry& m )ι->Log::Proto::LogEntryFile{
		Log::Proto::LogEntryFile proto;
		logEntry( m, proto );
		return proto;
	}
	α LogProto::LogEntryFile( const Logging::Entry& m, App::ProgramPK appPK, App::ProgInstPK instancePK )ι->Log::Proto::LogEntryFileExternal{
		Log::Proto::LogEntryFileExternal proto;
		logEntry( m, proto );
		proto.set_app_pk( appPK );
		proto.set_app_instance_pk( instancePK );
		return proto;
	}

	α LogProto::ToString( uuid id, string&& value )ι->Log::Proto::String{
		Log::Proto::String m;
		*m.mutable_id() = ToBytes( id );
		*m.mutable_value() = move( value );
		return m;
	}

	Ω guidToString( const string& guid )ι->string{
		using Protobuf::ToGuid;
		if( guid.size()!=16 )
			return Ƒ( "invalid guid size: {}", guid.size() );
		let v = ToString( ToGuid(guid) );
		return v.substr(v.size()-4);
	}

	using namespace Log::Proto;
	Ŧ debugStringLogEntry( const T& f )ι->string{
		string result = Ƒ( "[{}.{}.{}] {} {{",
			guidToString( f.template_id() ),
			ToString( (Jde::ELogLevel)f.level() ),
			ToString( (ELogTags)f.tags() ),
			Chrono::LocalTimeMilli( Protobuf::ToTimePoint(f.time()) ),
			//guidToString( f.file_id() ),
			//f.line(),
			//guidToString( f.function_id() ),
			f.user_pk()
		);
		for( auto& arg : f.args() )
			result += Ƒ( "{}, ", guidToString(arg) );
		result += "}";
		return result;
	}
	Ω debugStringExternal( const LogEntryFileExternal& f )ι->string{
		string result = debugStringLogEntry( f );
		result += Ƒ( ", app_pk: {}, app_instance_pk: {}", f.app_pk(), f.app_instance_pk() );
		return result;
	}
	α LogProto::DebugString( const FileEntry& f )ι->string{
		switch( f.value_case() ){
			using Type = FileEntry::ValueCase;
			case Type::kEntry:
				return debugStringLogEntry( f.entry() );
			break;
			case Type::kExternalEntry:
				return debugStringExternal( f.external_entry() );
			break;
			case Type::kApp:
				return Ƒ( "app: {{ id: {}, name: {} }}", f.app().id(), f.app().name() );
			break;
			case Type::kAppInstance:
				return Ƒ( "app_instance: {{ id: {}, app_id: {}, host_id: {}, pid: {} }}", f.app_instance().id(), f.app_instance().app_id(), f.app_instance().host_id(), f.app_instance().pid() );
			break;
			case Type::kHost:
				return Ƒ( "host: {{ id: {}, name: {} }}", f.host().id(), f.host().name() );
			break;
			case Type::kStr:
				return Ƒ( "[{}]{}", guidToString(f.str().id()), f.str().value() );
			default:
				return "Error";
			break;
		}
	}
	α LogProto::DebugString( const Log::Proto::LogEntryFile& f )ι->string{
		return debugStringLogEntry( f );
	}
	α LogProto::DebugString( const Log::Proto::LogEntryFileExternal& f )ι->string{
		return debugStringExternal( f );
	}
}
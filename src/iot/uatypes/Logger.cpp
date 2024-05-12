#include <jde/iot/uatypes/Logger.h>

#define var const auto
namespace Jde::Iot{
	constexpr array<sv,8> Categories{ "UANet", "UASecure", "UASession", "UAServer", "UAClient", "UAUser", "UASecurity", "UAEvent" };
	vector<sp<LogTag>> _tags = Logging::Tag( Categories );

	α Format( const char* format, va_list ap )ι->string{
		va_list ap_copy; va_copy( ap_copy, ap );
		var len = vsnprintf( 0, 0, format, ap_copy );
		string m; m.resize( len + 1 );
		vsnprintf( m.data(), len + 1, format, ap );
		m.resize( len );  // remove the NULL
		return m;
	}
	α Clear( UA_Logger* context )ι->void{}
	
	α UA_Log_Stdout_log( void *context, UA_LogLevel uaLevel, UA_LogCategory category, const char* file, const char* function, uint32_t line, const char *msg, va_list args )ι->void{
		var level = (ELogLevel)( (int)uaLevel/100-1 ); //level==UA_LOGLEVEL_DEBUG=200
		var tagName = Str::FromEnum( Categories, category );
		var tag = category<Categories.size() ? _tags[category] : AppTag();
		Logging::Log( Logging::Message{tag->Id, level, Jde::format("[{:x}]{}", (uint)context, Iot::Format(msg,args)), file, function, line}, tag, true, false );
	}

	Logger::Logger( Handle uaHandle )ι:
		UA_Logger{ UA_Log_Stdout_log, (void*)uaHandle, Clear }
	{}
}
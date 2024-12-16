#include <jde/opc/uatypes/Logger.h>

#define let const auto
namespace Jde::Opc{
	constexpr array<sv,14> EOpcLogTagstrings{ "iot",
		"uaNet", "uaSecure", "uaSession", "uaServer", "uaClient", "uaUser", "uaSecurity", "uaEvent", "uaPubSub", "uaDiscovery",
		"monitoring", "browse", "processingLoop" };

	α Format( const char* format, va_list ap )ι->string{
		va_list ap_copy; va_copy( ap_copy, ap );
		let len = vsnprintf( 0, 0, format, ap_copy );
		string m; m.resize( len + 1 );
		vsnprintf( m.data(), len + 1, format, ap_copy );
		va_end(ap_copy);
		m.resize( len );  // remove the NULL
		return m;
	}
	α Clear( UA_Logger* /*context*/ )ι->void{}

	α UA_Log_Stdout_log( void *context, UA_LogLevel uaLevel, UA_LogCategory category, const char* file, const char* function, uint32_t line, const char *m, va_list args )ι->void{
		let level = (ELogLevel)( (int)uaLevel/100-1 ); //level==UA_LOGLEVEL_DEBUG=200
		uint tag_ = 1ull << (uint)(33ull+category);
		const EOpcLogTags tag = (EOpcLogTags)( tag_ );

		Log( level, (ELogTags)tag, spdlog::source_loc{file, (int)line, function}, "[{:x}]{}", (uint)context, Opc::Format(m,args) );
	}

	Logger::Logger( Handle uaHandle )ι:
		UA_Logger{ UA_Log_Stdout_log, (void*)uaHandle, Clear }
	{}
}
namespace Jde{
	α Opc::LogTagParser( sv name )ι->optional<ELogTags>{
		optional<ELogTags> tag;
		for( uint i=0; !tag && i<EOpcLogTagstrings.size(); ++i )
			if( EOpcLogTagstrings[i]==name )
				tag = (ELogTags)(1ull << (32+i));
		return tag;
	}
}
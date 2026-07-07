#include <jde/fwk/log/log.h>
#include <boost/lexical_cast.hpp>
#ifdef _MSC_VER
	#include <crtdbg.h>
	#include <spdlog/spdlog.h>
	#include <spdlog/sinks/msvc_sink.h>
#endif
#include <jde/fwk/settings.h>
#include <jde/fwk/str.h>
#include "jde/fwk/log/logTags.h"
#include <jde/fwk/log/MemoryLog.h>

#define let const auto

namespace Jde{
	α initLoggers()ι->vector<up<Logging::ILogger>>{
		vector<up<Logging::ILogger>> y;
		y.reserve( 8 );
		y.push_back( mu<Logging::MemoryLog>() );
		Logging::UpdateCumulative( y );
		return y;
	}
	vector<up<Logging::ILogger>> _loggers = initLoggers();
	α Logging::Loggers()->const vector<up<ILogger>>&{ return _loggers; }
	α Logging::AddLogger( up<ILogger>&& logger )ι->ILogger*{ ASSERT(logger); _loggers.push_back(move(logger)); return _loggers.back().get(); }
	inline constexpr std::array<sv,7> ELogLevelStrings = { "Trace", "Debug", "Information", "Warning", "Error", "Critical", "None" };
}

α Jde::LogLevelStrings()ι->const std::array<sv,7>{ return ELogLevelStrings; }

α Jde::ToString( ELogLevel l )ι->string{
	return l==ELogLevel::NoLog ? "None" : FromEnum( ELogLevelStrings, l );
}
α Jde::ToLogLevel( sv l )ι->ELogLevel{
	let level = ToEnum<ELogLevel>( ELogLevelStrings, l );
	ASSERT( level.has_value() );
	return level.value_or( ELogLevel::Error );
}

namespace Jde::Logging{
	auto _pOnceMessages = mu<flat_map<uint,flat_set<string>>>(); std::shared_mutex OnceMessageMutex;
}

namespace Jde{
	α Logging::DestroyLoggers( bool terminate )->void{
		Logging::_pOnceMessages = nullptr;
		for( auto p=_loggers.begin(); p!=_loggers.end(); ){
			auto logger = move( *p );
			p = _loggers.erase( p );
			logger->Shutdown( terminate );
		}
	};

	α Logging::Init()ι->void{
		SetBreakLevel();
		Logging::Add<SpdLog>( "spd" );
		auto& memoryLogger = Logging::GetLogger<MemoryLog>();
		for( let& logger : _loggers ){
			if( dynamic_cast<MemoryLog*>(logger.get()) )
				continue;
			memoryLogger.Write( *logger.get() );
		}
		auto memory = Settings::FindObject( "/logging/memory/tags" );
		if( !memory || Json::FindEnum<ELogLevel>(*memory, "default", ToLogLevel).value_or(ELogLevel::NoLog)==ELogLevel::NoLog )
			_loggers.erase( _loggers.begin() );
		else
		 	_loggers.front()->SetLevels( *memory );
		Logging::UpdateCumulative( _loggers );
	}
}
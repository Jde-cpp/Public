#include <jde/fwk/log/log.h>
#include <boost/lexical_cast.hpp>
#ifdef _MSC_VER
	#include <crtdbg.h>
	#include <spdlog/spdlog.h>
	#include <spdlog/sinks/msvc_sink.h>
#endif
#include <jde/fwk/settings.h>
#include <jde/fwk/str.h>
#include <jde/fwk/log/MemoryLog.h>

#define let const auto

namespace Jde{
	α initLoggers()ι->vector<up<Logging::ILogger>>{
		vector<up<Logging::ILogger>> y;
		y.push_back(mu<Logging::MemoryLog>() );
		Logging::UpdateCumulative(y);
		return y;
	}
	vector<up<Logging::ILogger>> _loggers = initLoggers();
	α Logging::Loggers()->const vector<up<ILogger>>&{ return _loggers; }
	α Logging::AddLogger( up<ILogger>&& logger )->void{ _loggers.push_back( move(logger) ); }
	α Logging::MemoryLogger()ε->MemoryLog&{
		for( let& logger : _loggers )
			if( auto log = dynamic_cast<MemoryLog*>( logger.get() ) )
				return *log;
		THROW( "No MemoryLog found." );
	}
	inline constexpr std::array<sv,7> ELogLevelStrings = { "Trace", "Debug", "Information", "Warning", "Error", "Critical", "None" };
}

α Jde::LogLevelStrings()ι->const std::array<sv,7>{ return ELogLevelStrings; }

α Jde::ToString( ELogLevel l )ι->string{
	return FromEnum( ELogLevelStrings, l );
}
α Jde::ToLogLevel( sv l )ι->ELogLevel{
	return ToEnum<ELogLevel>( ELogLevelStrings, l ).value_or( ELogLevel::Error );
}

namespace Jde::Logging{
	auto _pOnceMessages = mu<flat_map<uint,flat_set<string>>>(); std::shared_mutex OnceMessageMutex;
	constexpr ELogTags _tags{ ELogTags::Settings };
}

namespace Jde{
	α Logging::DestroyLoggers( bool terminate )->void{
		TRACET( ELogTags::App, "Destroying Loggers" );
		Logging::_pOnceMessages = nullptr;
		for( auto p=_loggers.begin(); p!=_loggers.end(); ){
			auto logger = move(*p);
			p = _loggers.erase(p);
			logger->Shutdown( terminate );
		}
	};

	α Logging::Initialize()ι->void{
		_loggers.push_back( mu<SpdLog>() );
		for( let& logger : _loggers ){
			if( dynamic_cast<MemoryLog*>(logger.get()) )
				continue;
			MemoryLogger().Write( *logger.get() );
		}
		auto memory = Settings::FindObject("/logging/memory");
		if( !memory || Json::FindEnum<ELogLevel>(*memory, "default", ToLogLevel).value_or(ELogLevel::NoLog)==ELogLevel::NoLog )
			_loggers.erase( _loggers.begin() );
		UpdateCumulative( _loggers );
	}

/*	α SendStatus()ι->void
	{
		lg _{_statusMutex};
		vector<string> variables; variables.reserve( _status.details_size()+1 );
		_status.set_memory( IApplication::MemorySize() );
		ostringstream os;
		os << "Memory=" << _status.memory();
		for( int i=0; i<_status.details_size(); ++i )
			os << ";  " << _status.details(i);

		TRACET( Logging::_statusTag, "{}", os.str() );
		if( Logging::Server::Enabled() )
			Logging::Server::SetSendStatus();
		_lastStatusUpdate = Clock::now();
	}

	α Logging::SetStatus( const vector<string>& values )ι->void{
		{
			lg _{_statusMutex};
			_status.clear_details();
			for( let& value : values )
				_status.add_details( value );
		}
		let now = Clock::now();
		if( _lastStatusUpdate+10s<now )
			SendStatus();
	}
	α Logging::GetStatus()ι->up<Logging::Proto::Status>
	{
		lg _{_statusMutex};
		return mu<Proto::Status>( _status );
	}
*/
}
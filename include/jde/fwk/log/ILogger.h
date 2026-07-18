#pragma once
#include "logTags.h"
#include "../settings.h"
#include "../process/process.h"

#define Φ Γ auto
namespace Jde::Logging{

	Φ DestroyLoggers( bool terminate )->void;
	Φ Loggers()->const vector<up<ILogger>>&;
	Ŧ GetLogger()ε->T&;
	Ŧ FindLogger()ι->T*;
	Φ AddLogger( up<ILogger>&& logger )ι->ILogger*;
	Φ Init()ι->void;

	struct Entry;
	struct Γ ILogger : LogTags, IShutdown{
		ILogger( const jobject& o ): LogTags( o ){}
		virtual ~ILogger(){} //important
		β Write( const Entry& m )ι->void=0;
		β Write( const Entry& m, uint32 appPK, uint32 instancePK )ι->void=0;
	};

	template<class T, class... Args>
	α Add( sv configName, Args&& ...args )ι->T*{
		T* y{};
		if( const auto settings = Settings::FindObject( Ƒ("/logging/{}", configName) ); settings ){
			try{
				y = (T*)AddLogger( mu<T>(*settings, FWD(args)...) );
			}
			catch( exception& )
			{}
		}
		return y;
	}
}
namespace Jde{
	Ŧ Logging::FindLogger()ι->T*{
		for( auto& logger : Loggers() ){
			if( auto log = dynamic_cast<T*>( logger.get() ) )
				return log;
		}
		return nullptr;
	}
	Ŧ Logging::GetLogger()ε->T&{
		auto p = FindLogger<T>();
		if( !p )
			throw std::runtime_error( Ƒ("Logger of type {} not found.", typeid(T).name()) );
		return *p;
	}
}
#undef Φ
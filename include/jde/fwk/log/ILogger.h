#pragma once
#ifndef LOGGER_H
#define LOGGER_H
#include "../settings.h"
#include "logTags.h"

namespace Jde::Logging{
	struct Entry;
	struct Γ ILogger : LogTags, IShutdown{
		ILogger( jobject o ): LogTags( move(o) ){}
		Ṫ Init( sv configName )ι->void;
		virtual ~ILogger(){} //important
		β Write( const Entry& m )ι->void=0;
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
#endif
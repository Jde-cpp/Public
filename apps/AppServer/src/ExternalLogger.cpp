#include "ExternalLogger.h"
#include <jde/app/IApp.h>
#include <jde/app/shared/StringCache.h>
#include <jde/app/shared/proto/App.FromClient.h>
#include "WebServer.h"
#include "LogData.h"

namespace Jde::App{
	α ExternalLogger::Log( Logging::ExternalMessage&& m, SL )ι->void{
		Log( m );
	}
	α ExternalLogger::Log( const Logging::ExternalMessage& m, const vector<string>* args, SL )ι->void{
		if( _minLevel==ELogLevel::NoLog || m.Level<_minLevel )
			return;
		Server::BroadcastLogEntry( 0, Server::GetAppPK(), _appClient->InstancePK(), m, *args );
		try{
			if( StringCache::AddMessage(m.MessageId, string{m.MessageView}) )
				Server::SaveString( Proto::FromClient::EFields::MessageId, (uint32)m.MessageId, string{m.MessageView} );
			if( StringCache::AddFile(m.FileId, m.File) )
				Server::SaveString( Proto::FromClient::EFields::FileId, (uint32)m.FileId, m.File );
			if( StringCache::AddFunction(m.FunctionId, m.Function) )
				Server::SaveString( Proto::FromClient::EFields::FunctionId, (uint32)m.FunctionId, m.Function );
			SaveMessage( Server::GetAppPK(), _appClient->InstancePK(), FromClient::ToLogEntry(m), args );
		}
		catch( const IException& ){}
	}
	α ExternalLogger::SetMinLevel( ELogLevel level )ι->void{
		_minLevel = level;
		Server::BroadcastAppStatus();
	}
}
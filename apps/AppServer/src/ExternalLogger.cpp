#include "ExternalLogger.h"
#include <jde/app/IApp.h>
#include <jde/app/shared/StringCache.h>
#include <jde/app/shared/proto/App.FromClient.h>
#include "WebServer.h"
#include "LogData.h"

namespace Jde::App{
	#define let const auto
	α ExternalLogger::Write( Logging::Entry&& m )ι->void{
		Write( m );
	}
	α ExternalLogger::Write( const Logging::Entry& m )ι->void{
		if( _minLevel==ELogLevel::NoLog || m.Level<_minLevel )
			return;
		Server::BroadcastLogEntry( 0, Server::GetAppPK(), _appClient->InstancePK(), m, m.Arguments );
		try{
/*		if( let id{m.Id()}; StringCache::AddMessage(id, string{m.Text}) )
				Server::SaveString( Proto::FromClient::EFields::MessageId, id, m.Text );
			if( let id{m.FileId()}; StringCache::AddFile(id, string{m.File()}) )
				Server::SaveString( Proto::FromClient::EFields::FileId, id, string{m.File()} );
			if( let id{m.FunctionId()}; StringCache::AddFunction(id, string{m.Function()}) )
				Server::SaveString( Proto::FromClient::EFields::FunctionId, id, string{m.Function()} );
			SaveMessage( Server::GetAppPK(), _appClient->InstancePK(), FromClient::ToLogEntry(m) );
*/
		}
		catch( const IException& ){}
	}
}
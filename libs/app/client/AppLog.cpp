#include "AppLog.h"
#include <jde/app/client/AppClientSocketSession.h>
#include <jde/app/client/appClient.h>
#include <jde/app/shared/StringCache.h>
#include <jde/app/shared/proto/App.FromClient.h>

namespace Jde::App::Client{
	using namespace Jde::Logging;
	α AppLog::Destroy( SL )ι->void{
		CloseSocketSession();
	}
	α AppLog::Log( const ExternalMessage& m, const vector<string>* /*args*/, SL /*sl*/ )ι->void{
		Proto::FromClient::Transmission t;
		if( StringCache::AddFile(m.FileId, m.File) )
			FromClient::AddStringField( t, Proto::FromClient::EFields::File, m.FileId, m.File );
		if( StringCache::AddFunction(m.FileId, m.Function) )
			FromClient::AddStringField( t, Proto::FromClient::EFields::Function, m.FileId, m.Function );
		if( StringCache::AddMessage(m.FileId, string{m.MessageView}) )
			FromClient::AddStringField( t, Proto::FromClient::EFields::MessageField, m.MessageId, string{m.MessageView} );
		//[&]->LogAwait::Task { co_await LogAwait{ m, args, sl }; }();
	}
	α AppLog::SetMinLevel(ELogLevel level)ι->void{
		UpdateStatus();
		_minLevel = level;
	}
}

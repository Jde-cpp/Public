#include "AppLog.h"
#include <jde/app/client/AppClientSocketSession.h>
#include <jde/app/client/appClient.h>
#include <jde/app/shared/StringCache.h>
#include <jde/app/shared/proto/App.FromClient.h>

namespace Jde::App::Client{
	using namespace Jde::Logging;
	α AppLog::Write( const Logging::Entry& m )ι->void{
		Proto::FromClient::Transmission t;
		if( StringCache::AddFile(m.FileId(), string{m.File()}) )
			FromClient::AddStringField( t, Proto::FromClient::EFields::File, m.FileId(), string{m.File()} );
		if( StringCache::AddFunction(m.FileId(), string{m.Function()}) )
			FromClient::AddStringField( t, Proto::FromClient::EFields::Function, m.FileId(), string{m.Function()} );
		if( StringCache::AddMessage(m.FileId(), m.Text) )
			FromClient::AddStringField( t, Proto::FromClient::EFields::MessageField, m.Id(), m.Text );
		//[&]->LogAwait::Task { co_await LogAwait{ m, args, sl }; }();
	}
}
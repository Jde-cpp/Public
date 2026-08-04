#include <jde/app/log/LogQLAwait.h>
#include <boost/uuid/uuid_io.hpp>
#include <jde/fwk/chrono.h>
#include "LogAwait.h"
#include "jde/fwk/log/log.h"

#define let const auto

namespace Jde::App{
	α LogQLAwait::Execute()ι->TAwait<ArchiveFile>::Task{
		try{
			auto archive = co_await LogAwait{ _ql, _sl };
			Resume( archive.ToJson(_ql) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}
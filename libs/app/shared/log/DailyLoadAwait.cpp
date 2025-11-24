#include <jde/app/log/DailyLoadAwait.h>
#include <jde/fwk/co/LockKey.h>
#include <jde/fwk/io/FileAwait.h>
#include <jde/fwk/io/protobuf.h>
#include <jde/app/log/ProtoLog.h>

namespace Jde::App{
	α DailyLoadAwait::Execute()ι->TAwait<CoLockGuard>::Task{
		Read( co_await LockKeyAwait{_file.string()} );
	}
	α DailyLoadAwait::Read( CoLockGuard )ι->TAwait<string>::Task{
		auto log = Logging::GetLogger<App::ProtoLog>();
		try{
			auto y = log ? log->Entries() : vector<App::Log::Proto::FileEntry>{};
			string content = co_await IO::ReadAwait( _file );
			auto fileContent = App::ProtoLog().Deserialize( move(content) );
			y.insert( y.end(), make_move_iterator(fileContent.begin()), make_move_iterator(fileContent.end()) );
			Resume( move(y) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}
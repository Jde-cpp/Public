#include "jde/fwk/io/file.h"
#include <jde/app/log/DailyLoadAwait.h>
#include <jde/fwk/co/LockKey.h>
#include <jde/fwk/io/FileAwait.h>
#include <jde/fwk/io/protobuf.h>
#include <jde/app/log/ProtoLog.h>
#include <jde/app/proto/LogProto.h>

#define let const auto
namespace Jde::App{
	constexpr ELogTags _tags = ELogTags::ExternalLogger;
	α DailyLoadAwait::Execute()ι->TAwait<CoLockGuard>::Task{
		Read( co_await LockKeyAwait{_file.string()} );
	}
	α DailyLoadAwait::Read( CoLockGuard )ι->TAwait<string>::Task{
		auto log = Logging::FindLogger<App::ProtoLog>();
		try{
			auto y = log ? log->Entries() : vector<App::Log::Proto::FileEntry>{};
			TRACE( "Memory item count: {}", y.size() );
			if( fs::exists(_file) ){
				auto content = co_await IO::ReadAwait( _file );
				auto fileContent = App::ProtoLog::Deserialize( move(content) );
				string save;
				for( auto& entry : fileContent )
					save +=  LogProto::DebugString( entry ) + "\n";
				TRACE( "DailyFile item count: {}", fileContent.size() );
				y.insert( y.end(), make_move_iterator(fileContent.begin()), make_move_iterator(fileContent.end()) );
			}
			Resume( move(y) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}
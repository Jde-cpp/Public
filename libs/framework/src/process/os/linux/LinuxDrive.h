#pragma once
#include <signal.h>
#include <aio.h>
#include <jde/framework/io/FileAwait.h>
#include "../../Framework/source/io/IDrive.h"
#include "../../Framework/source/coroutine/Awaitable.h"
#include <jde/framework/io/file.h>

//using namespace Jde::Coroutine;
namespace Jde::IO{
	//constexpr int CompletionSignal{ SIGUSR2 };
	//α AioCompletionHandler( int signo, siginfo_t *info, void *context )ι->void;
}

namespace Jde::IO::Drive{
	struct NativeDrive final: public IDrive{
		//void Recursive2( path dir )ε;
		α Recursive( const fs::path& dir, SRCE )ε->flat_map<string,up<IDirEntry>> override;
		α Get( const fs::path& path )ε->up<IDirEntry> override;
		α Save( const fs::path& path, const vector<char>& bytes, const IDirEntry& dirEntry )ε->up<IDirEntry> override;
		α CreateFolder( const fs::path& path, const IDirEntry& dirEntry )ε->up<IDirEntry> override;

		α Trash( const fs::path& path )ι->void override;
		α Load( const IDirEntry& dirEntry )ε->vector<char> override;
		α Remove( const fs::path& )ε->void override;
		α TrashDisposal( TimePoint /*latestDate*/ )ε->void override{ THROW("Not Implemented"); };
		α Restore( sv /*name*/ )ε->void override{ THROW("Not Implemented"); };
		α SoftLink( const fs::path& existingFile, const fs::path& newSymLink )ε->void override;
	};
}
#pragma once
#include <signal.h>
#include <aio.h>
#include "../../Framework/source/io/DiskWatcher.h"
#include "../../Framework/source/coroutine/Awaitable.h"
#include <jde/framework/io/file.h>

using namespace Jde::Coroutine;
namespace Jde::IO
{
	struct LinuxChunk final : IFileChunkArg
	{
		LinuxChunk( FileIOArg& pIOArg, uint index )ι;
		//uint StartIndex()const ι override;
		//void SetStartIndex( uint i )ι override;
		//uint Bytes()const ι override{ return _linuxArg.aio_nbytes; } virtual void SetBytes( uint x )ι override{ _linuxArg.aio_nbytes=x; }
		//void SetEndIndex( uint i )ι override;
		//void SetFileIOArg( FileIOArg* p )ι override{ _fileIOArgPtr=p; }
		//HFile Handle()ι override{ return _linuxArg.aio_fildes; };
		//void Process( int handle )ι override;
		//optional<bool> Complete()ι;
	private:
		//aiocb _linuxArg;
	};

/*	struct LinuxDriveWorker final : DriveWorker
	{
		//static void IOHandler( int s )ι;
	//	static void AioSigHandler( int sig, siginfo_t* pInfo, void* pContext )ι;
	};*/
}
namespace Jde::IO::Drive
{
	struct NativeDrive final: public IDrive
	{
		//void Recursive2( path dir )ε;
		flat_map<string,IDirEntryPtr> Recursive( const fs::path& dir, SRCE )ε override;
		IDirEntryPtr Get( const fs::path& path )ε override;
		IDirEntryPtr Save( const fs::path& path, const vector<char>& bytes, const IDirEntry& dirEntry )ε override;
		IDirEntryPtr CreateFolder( const fs::path& path, const IDirEntry& dirEntry )ε override;

		α Trash( const fs::path& path )ι->void override;
		α Load( const IDirEntry& dirEntry )ε->sp<vector<char>> override;
		α Remove( const fs::path& )ε->void override;
		α TrashDisposal( TimePoint /*latestDate*/ )ε->void override{ THROW("Not Implemented"); };
		α Restore( sv name )ε->void override{ THROW("Not Implemented"); };
		α SoftLink( const fs::path& existingFile, const fs::path& newSymLink )ε->void override;
	};
}
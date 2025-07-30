#pragma once
#include <jde/framework/process/os/windows/WindowsHandle.h>
#include <jde/framework/coroutine/Task.h>
#include "../../Framework/source/io/FileCo.h"
#include "../../Framework/source/io/IDrive.h"
#include "../../Framework/source/threading/Worker.h"

namespace Jde::IO
{
	using namespace Coroutine;

	struct FileChunkArg : IFileChunkArg
	{
		FileChunkArg( FileIOArg& arg, uint index ):
			IFileChunkArg{ arg, index },
			EndIndex{ std::min((Index+1)*DriveWorker::ChunkSize(), FileArg().Size()) },
			Overlap{ .Pointer=(PVOID)(Index*DriveWorker::ChunkSize()), .hEvent=this },
			Bytes{ EndIndex-Overlap.Offset }
		{
		}
		α Buffer(){ return FileArg().Data()+(size_t)Overlap.Pointer; }
		α StartIndex(){ return (uint)Overlap.Pointer; }
		uint EndIndex;
		::OVERLAPPED Overlap;
		uint Bytes;
	};

	struct WinDriveWorker : Threading::IQueueWorker<FileIOArg*,WinDriveWorker>{
		using base=IQueueWorker<FileIOArg*,WinDriveWorker>;
		WinDriveWorker():base{}{}
		Ω Remove( FileIOArg* pArg )ι->void;
		α Poll()ι->optional<bool> override;
		α HandleRequest( FileIOArg*&& x )ι->void override;
		α SetWorker( FileIOArg*& p )ι->void override{ p->SetWorker( shared_from_this() ); }
		α Shutdown( bool /*terminate*/ )ι->void override{}
		static constexpr sv Name{ "drive" };

	private:
		vector<FileIOArg*> _args; std::atomic_flag _argMutex;
	};
}
namespace Jde::IO::Drive
{
	//struct IDrive;
	//auto CloseFile = [](HANDLE h){ if( h && !::CloseHandle(h) ) WARN( "CloseHandle returned '{}'"sv, ::GetLastError() ); };
	/*
	struct DriveArg //: boost::noncopyable
	{
		DriveArg( fs::path&& path, sp<IDrive> pDrive, bool vec ):
			IsRead{ true },
			Path{ move(path) },
			DrivePtr{ pDrive }
		{
			Overlaps.reserve( 5 );
			if( vec )
				Buffer = make_shared<vector<char>>();
			else
				Buffer = make_shared<string>();
		}
		DriveArg( fs::path&& path, sp<IDrive> pDrive, sp<vector<char>> pVec ):
			Path{ move(path) }, DrivePtr{ pDrive }, Buffer{ pVec }
		{}
		DriveArg( fs::path&& path, sp<IDrive> pDrive, sp<string> pData ):
			Path{ move(path) }, DrivePtr{ pDrive }, Buffer{ pData }
		{}
		void Send( up<::OVERLAPPED> pOverlapped )ι;
		void SetWorker( sp<Threading::IWorker> p ){ _pWorkerKeepAlive=p; }
		bool IsRead{false};
		fs::path Path;
		sp<IDrive> DrivePtr;
		std::variant<sp<vector<char>>,sp<string>> Buffer;
		HandlePtr FileHandle;
		vector<up<::OVERLAPPED>> Overlaps;
		vector<up<::OVERLAPPED>> OverlapsOverflow;//std::queue not working for some reason in windows.
		coroutine_handle<Coroutine::Task2::promise_type> CoHandle;
	private:
		sp<Threading::IWorker> _pWorkerKeepAlive;
	};
	*/
	/*
	struct DriveAwaitable : Coroutine::IAwaitable
	{
		using base=Coroutine::IAwaitable;
		DriveAwaitable( fs::path&& path, bool vector, sp<IDrive> pDrive )ι:_arg{ move(path), pDrive, vector }{ DBG("DriveAwaitable::Read"sv); }
		DriveAwaitable( fs::path&& path, sp<vector<char>> data, sp<IDrive> pDrive )ι:_arg{ move(path), pDrive, data }{ DBG("DriveAwaitable::Write"sv); }
		DriveAwaitable( fs::path&& path, sp<string> data, sp<IDrive> pDrive )ι:_arg{ move(path), pDrive, data }{ DBG("her3e"sv); }
		bool await_ready()ι override;
		void await_suspend( typename base::THandle h )ι override;//{ base::await_suspend( h ); _pPromise = &h.promise(); }
		TaskResult await_resume()ι override;
	private:
		std::exception_ptr ExceptionPtr;
		DriveArg _arg;
	};
	*/
	struct Γ WindowsDrive final : IDrive//TODO remove Γ
	{
		α Get( const fs::path& path )ε->up<IDirEntry> override;
		α Recursive( const fs::path& dir, SRCE )ε->flat_map<string,up<IDirEntry>> override;
		up<IDirEntry> Save( const fs::path& path, const vector<char>& bytes, const IDirEntry& dirEntry )ε override;
		α CreateFolder( const fs::path& path, const IDirEntry& dirEntry )ε->up<IDirEntry> override;
		void Remove( const fs::path& /*path*/ )ε override{THROW( "Not Implemented" );}
		void Trash( const fs::path& /*path*/ )ε override{THROW( "Not Implemented" );}
		void TrashDisposal( TimePoint /*latestDate*/ )override{THROW( "Not Implemented" );}
		α Load( const IDirEntry& dirEntry )ε->vector<char> override;
		void Restore( sv )ε override{ THROW("Not Implemented"); }
		void SoftLink( const fs::path& from, const fs::path& to )ε override;
		//DriveAwaitable Read( fs::path&& path, bool vector=true )ι{ return DriveAwaitable{move(path), vector,shared_from_this()}; }
		//DriveAwaitable Write( fs::path&& path, sp<vector<char>> data )ι{ return DriveAwaitable{move(path), data, shared_from_this()}; }
		//DriveAwaitable Write( fs::path&& path, sp<string> data )ι{ return DriveAwaitable{move(path), data, shared_from_this()}; }
	};
}
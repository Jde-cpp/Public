#include "LinuxDrive.h"
#include <unistd.h>
#include <jde/framework/io/file.h>
#include "../../Framework/source/Cache.h"

#define let const auto
namespace Jde::IO{
	constexpr ELogTags _tags{ ELogTags::IO };

	Drive::NativeDrive _native;
	α Native()ι->IDrive&{ return _native; }

	α FileIOArg::Open()ε->void
	{
		Handle = ::open( Path.string().c_str(), O_NONBLOCK | (IsRead ? O_RDONLY : O_WRONLY|O_CREAT|O_TRUNC), 0666 );
		if( Handle==-1 )
		{
			Handle = 0;
			if( !IsRead && errno==ENOENT )
			{
				fs::create_directories( Path.parent_path() );
				Information{ _tags, "Created dir {}", Path.parent_path().string() };
				return Open();
			}
			THROW_IFX( IsRead /*|| errno!=EACCES*/, IOException(move(Path), errno, "open", _sl) );
		}
		if( IsRead )
		{
			struct stat st;
			THROW_IFX( ::fstat( Handle, &st )==-1, IOException(move(Path), errno, "fstat", _sl) );
			std::visit( [size=st.st_size](auto&& b){b->resize(size);}, Buffer );
		}
	}

	α FileIOArg::Send( coroutine_handle<Task::promise_type> h )ι->void{
		CoHandle = h;
		CoroutinePool::Resume( move(CoHandle) );
	}

	α DriveAwaitable::await_resume()ι->AwaitResult{
		if( ExceptionPtr )
			return AwaitResult{ move(ExceptionPtr) };
		if( _cache && Cache::Has(_arg.Path) )
		{
			sp<void> pVoid = std::visit( [](auto&& x){return (sp<void>)x;}, _arg.Buffer );
			return AwaitResult{ move(pVoid) };
		}
		try
		{
			let size = _arg.Size();
			auto pData = std::visit( [](auto&& x){return x->data();}, _arg.Buffer );
			auto pEnd = pData+size;
			let chunkSize = DriveWorker::ChunkSize();
			let count = size/chunkSize+1;
			for( uint32 i=0; i<count; ++i )
			{
				auto pStart = pData+i*chunkSize;
				auto chunkCount = std::min<ptrdiff_t>( chunkSize, pEnd-pStart );
				if( _arg.IsRead )
				{
					THROW_IFX( ::read(_arg.Handle, pStart, chunkCount)==-1, IOException(_arg.Path, (uint)errno, "read()") );
				}
				else
					THROW_IFX( ::write(_arg.Handle, pStart, chunkCount)==-1, IOException(_arg.Path, (uint)errno, "write()") );
			}
			::close( _arg.Handle );
			sp<void> pVoid = std::visit( [](auto&& x){return (sp<void>)x;}, _arg.Buffer );
			if( _cache )
				Cache::Set( _arg.Path, pVoid );

			return AwaitResult{ move(pVoid) };
		}
		catch( IException& e ){
			return AwaitResult{ e.Move() };
		}
	}


/*
	α DriveAwaitable::await_suspend( typename base::THandle h )ι->void
	{
		base::await_suspend( h );
		CoroutinePool::Resume( move(h) );
	}
*/
/*	HFile FileIOArg::SubsequentHandle()ι
	{
		auto h = Handle;
		if( h )
			Handle = 0;
		else
		{
			h = ::open( Path.string().c_str(), O_ASYNC | O_NONBLOCK | (IsRead ? O_RDONLY : O_WRONLY|O_CREAT|O_TRUNC) );
			Debug( _tags, "[{}]{}"sv, h, Path.string().c_str() );
		}
		return h;
	}
	*/
/*	up<IFileChunkArg> FileIOArg::CreateChunk( uint startIndex, uint endIndex )ι
	{
		return make_unique<LinuxFileChunkArg>( *this, startIndex, endIndex );
	}
*/
/*	α LinuxDriveWorker::AioSigHandler( int sig, siginfo_t* pInfo, void* pContext )ι->void
	{
		auto pChunkArg = (IFileChunkArg*)pInfo->si_value.sival_ptr;
		auto& fileArg = pChunkArg->FileArg();
		Debug( _tags, "Processing {}"sv, pChunkArg->index );
		if( fileArg.HandleChunkComplete(pChunkArg) )
		{
		//	if( ::close(pChunkArg->Handle())==-1 )
			fileArg.CoHandle.promise().get_return_object().SetResult( IOException{fileArg.Path, (uint)errno, "close"} );
			CoroutinePool::Resume( move(fileArg.CoHandle) );
			//delete pFileArg;
		}
	}
*/
/*	LinuxFileChunkArg::LinuxFileChunkArg( FileIOArg& ioArg, uint start, uint length )ι:
		IFileChunkArg{ ioArg }
	{
		//_linuxArg.aio_fildes = ioArg.FileHandle;
		_linuxArg.aio_lio_opcode = ioArg.IsRead ? LIO_READ : LIO_WRITE;
		_linuxArg.aio_reqprio = 0;
		_linuxArg.aio_buf = ioArg.Data()+start;
		_linuxArg.aio_nbytes = length;
		_linuxArg.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
		_linuxArg.aio_sigevent.sigev_signo = DriveWorker::Signal;
		_linuxArg.aio_sigevent.sigev_value.sival_ptr = this;
	}
	α LinuxFileChunkArg::Process()ι->void
	{
		Debug( _tags, "Sending chunk '{}' - handle='{}'"sv, index, handle );
		_linuxArg.aio_fildes = handle;
		let result = FileArg().IsRead ? ::aio_read( &_linuxArg ) : ::aio_write( &_linuxArg ); ERR_IF( result == -1, "aio({}) read={} returned false - {}", FileArg().Path.string(), FileArg().IsRead, errno );
		//let result = ::aio_write( &_linuxArg );
		ERR_IF( result == -1, "aio({}) read={} returned false - {}", FileArg().Path.string(), FileArg().IsRead, errno );
		//return result!=-1;
	}
*/
}
namespace Jde::IO::Drive
{
	α GetAttributes( fs::path path )->tuple<TimePoint,TimePoint,uint>
	{
		struct stat attrib;
		stat( path.string().c_str(), &attrib );
		let size = attrib.st_size;

		const TimePoint modifiedTime = Clock::from_time_t( attrib.st_mtim.tv_sec );//+std::chrono::nanoseconds( attrib.st_mtim.tv_nsec );TODO
		const TimePoint accessedTime = Clock::from_time_t( attrib.st_atim.tv_sec );//+std::chrono::nanoseconds( attrib.st_atim.tv_nsec );
		return make_tuple( modifiedTime,accessedTime, size );
	}
	struct DirEntry : IDirEntry
	{
		DirEntry( const fs::path& path ):
			DirEntry( fs::directory_entry(path) )
		{
			//LoadNativeDrive();//TODO Remove
		}
		DirEntry( const fs::directory_entry& entry )
		{
			let path = Path = entry.path();
			let status = entry.status();
			if( fs::is_directory(status) )
				Flags = EFileFlags::Directory;

			let& [modified, accessed, size] = GetAttributes( entry );
			Size = size;
			ModifiedTime = modified;
			AccessedTime = accessed;
		}
	};
	α NativeDrive::Get( const fs::path& path )ε->IDirEntryPtr
	{
		sp<const IDirEntry> pEntry = make_shared<const DirEntry>( path );
		return pEntry;
	}
	α NativeDrive::Recursive( const fs::path& dir, SL sl )ε->flat_map<string,IDirEntryPtr>
	{
		CHECK_PATH( dir, sl );
		let dirString = dir.string();
		flat_map<string,IDirEntryPtr> entries;

		std::function<void(const fs::directory_entry&)> fnctn;
		fnctn = [&dirString, &entries, &fnctn]( const fs::directory_entry& entry )
		{
			let status = entry.status();
			let relativeDir = entry.path().string().substr( dirString.size()+1 );

			sp<DirEntry> pEntry;
			if( fs::is_directory(status) || fs::is_regular_file(status) )
			{
				entries.emplace( relativeDir, make_shared<DirEntry>(entry.path()) );
				if( fs::is_directory(status) )
					FileUtilities::ForEachItem( entry.path(), fnctn );
			}
		};
		FileUtilities::ForEachItem( dir, fnctn );

		return entries;
	}
	α to_timespec( const TimePoint& time )->timespec
	{
		let sinceEpoch = time.time_since_epoch();
		let total = duration_cast<std::chrono::nanoseconds>( duration_cast<std::chrono::nanoseconds>( sinceEpoch )-duration_cast<std::chrono::seconds>( sinceEpoch ) ).count();

		return { Clock::to_time_t(time), total };
	}
	α NativeDrive::CreateFolder( const fs::path& dir, const IDirEntry& dirEntry )ε->IDirEntryPtr
	{
		fs::create_directory( dir );
		if( dirEntry.ModifiedTime.time_since_epoch()!=Duration::zero() )
		{
			//let [createTime, modifiedTime, size] = GetTimes( dirEntry );
			let modifiedTime = to_timespec( dirEntry.ModifiedTime );
			let accessedTime = dirEntry.AccessedTime.time_since_epoch()==Duration::zero() ? modifiedTime : to_timespec( dirEntry.AccessedTime );
			timespec values[] = {accessedTime, modifiedTime};
			if( !utimensat(AT_FDCWD, dir.string().c_str(), values, 0) )
				Warning( _tags, "utimensat returned {} on {}"sv, errno, dir.string() );
		}
		return make_shared<DirEntry>( dir );
	}
	α NativeDrive::Save( const fs::path& path, const vector<char>& bytes, const IDirEntry& dirEntry )ε->IDirEntryPtr
	{
		IO::FileUtilities::SaveBinary( path, bytes );
		if( dirEntry.ModifiedTime.time_since_epoch()!=Duration::zero() )
		{
			let modifiedTime = to_timespec( dirEntry.ModifiedTime );
			let accessedTime = dirEntry.AccessedTime.time_since_epoch()==Duration::zero() ? modifiedTime : to_timespec( dirEntry.AccessedTime );
			timespec values[] = {accessedTime, modifiedTime};
			if( utimensat(AT_FDCWD, path.string().c_str(), values, 0) )
				Warning( _tags, "utimensat returned {} on {}"sv, errno, path.string() );
		}
		return make_shared<DirEntry>( path );
	}

	//VectorPtr<char> NativeDrive::Load( path path )ε
	α NativeDrive::Load( const IDirEntry& dirEntry )ε->sp<vector<char>>{//fs::filesystem_error, IOException
		return IO::FileUtilities::LoadBinary( dirEntry.Path );
	}

	α NativeDrive::Remove( const fs::path& path )ε->void
	{
		Debug( _tags, "Removing '{}'."sv, path.string() );
		fs::remove( path );
	}
	α NativeDrive::Trash( const fs::path& path )ι->void
	{
		Debug( _tags, "Trashing '{}'."sv, path.string() );

		let result = system( Jde::format("gio trash {}", path.string()).c_str() );
		Debug( _tags, "Trashing '{}' returned '{}'."sv, path.string(), result );
	}
	α NativeDrive::SoftLink( const fs::path& existingFile, const fs::path& newSymLink )ε->void
	{
		let result = ::symlink( existingFile.string().c_str(), newSymLink.string().c_str() );
		THROW_IF( result!=0, "symlink creating '{}' referencing '{}' failed ({}){}.", newSymLink.string(), existingFile.string(), result, errno );
		Debug( _tags, "Created symlink '{}' referencing '{}'."sv, newSymLink.string(), existingFile.string() );
	}

/*	bool DriveWorker::Poll()ι
	{
		return base::Poll() || Args.size();//handle new queue item, from co_await Read() || currently handling item
	}
*/
	// α DriveWorker::HandleRequest( FileIOArg&& arg )ι->void
	// {
	// 	auto pArg = &Args.emplace_back( move(arg) );
	// 	let size = std::visit( [](auto&& x){return x->size();}, pArg->Buffer );
	// 	for( uint i=0; i<size; i+=ChunkSize() )
	// 	{
	// 		auto pChunkArg = make_unique<LinuxFileChunkArg>( pArg, i, std::min(DriveWorker::ChunkSize(), size) );
	// 		//Threading::AtomicGuard l{ pArg->Mutex };
	// 		if( pArg->Overlaps.size()<ThreadCount )
	// 			pArg->Send( move(pChunkArg) );
	// 		else
	// 			pArg->OverlapsOverflow.emplace_back( move(pChunkArg) );
	// 	}
	// }
/*	α FileIOArg::Send( up<IFileChunkArg> pChunkArg )ι->void
	{
		if( pChunkArg->Process() )
			Overlaps.emplace_back( move(pChunkArg) );
	}*/
}
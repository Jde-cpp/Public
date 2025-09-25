//#include <unistd.h>
#include <aio.h>
#include "LinuxDrive.h"
#include <jde/framework/io/file.h>
#include <jde/framework/io/FileAwait.h>
#include <jde/framework/thread/execution.h>
#include "../../../../../../Framework/source/Cache.h"
#include "../../../../../../Framework/source/coroutine/Coroutine.h"

#define let const auto
namespace Jde::IO{
	constexpr ELogTags _tags{ ELogTags::IO };

	struct LinuxChunk final : IFileChunkArg{
#define StartIndex index*ChunkByteSize()
		LinuxChunk( sp<FileIOArg> arg, uint index )ι:
			IFileChunkArg{ arg, index },
			CB{
				arg->Handle,
				arg->IsRead ? LIO_READ : LIO_WRITE,
				0,
				arg->Data()+StartIndex,
				std::min( StartIndex+ChunkByteSize(), arg->Size() )-index*ChunkByteSize(),
				{ {}, SIGUSR2, SIGEV_SIGNAL }
			}
		{}
#undef StartIndex
		aiocb64 CB;
	};


	α FileIOArg::Open( bool create )ε->void{
		auto flags = O_NONBLOCK | (IsRead ? O_RDONLY : O_WRONLY  | O_TRUNC);
		if( create && !IsRead )
			flags |= O_CREAT;
		Handle = ::open( Path.string().c_str(), flags, 0666 );
		if( Handle==-1 ){
			Handle = 0;
			if( !IsRead && errno==ENOENT ){
				fs::create_directories( Path.parent_path() );
				Information{ _tags, "Created dir {}", Path.parent_path().string() };
				return Open( create );
			}
			THROW_IFX( IsRead /*|| errno!=EACCES*/, IOException(move(Path), errno, "open", _sl) );
		}
		if( IsRead ){
			struct stat st;
			THROW_IFX( ::fstat( Handle, &st )==-1, IOException(move(Path), errno, "fstat", _sl) );
			std::visit( [size=st.st_size](auto&& b){b.resize(size);}, Buffer );
		}
	}

	Ω send( sp<FileIOArg> op )ι->void{
		up<IFileChunkArg> chunk;
		{
			lg l{ op->ChunkMutex };
			if( !op->Chunks.size() )
				return;
			chunk = move( op->Chunks.front() );
			op->Chunks.pop();
		}
		Trace{ ELogTags::Test, "aio_{} index: {}", chunk->IsRead() ? "read" : "write", chunk->Index };
		LinuxChunk& lchunk = dynamic_cast<LinuxChunk&>( *chunk );
		lchunk.CB.aio_sigevent.sigev_value.sival_ptr = chunk.release();
		if( let result = op->IsRead ? ::aio_read64( &lchunk.CB ) : ::aio_write64( &lchunk.CB ); result == -1 ){
			chunk = up<IFileChunkArg>( (IFileChunkArg*)lchunk.CB.aio_sigevent.sigev_value.sival_ptr );
			lchunk.FileArg()->PostExp( move(chunk), errno, Ƒ("aio_{} index: {}\n", chunk->IsRead() ? "read" : "write", chunk->Index) );
		}
	}

	α FileIOArg::Send( HCo h )ι->void{
		_coHandle = h;
		let threadSize = ThreadSize();
		let totalBytes = Size();
		let chunkByteSize = ChunkByteSize();
		auto sp = shared_from_this();
		{
			lg l{ ChunkMutex };
			ChunksToSend = totalBytes/chunkByteSize+1;
			for( uint i=0; i*chunkByteSize<totalBytes; ++i )
				Chunks.emplace( mu<LinuxChunk>(sp, i) );
			TRACE( "[{}] chunks = {}", Path.string(), ChunksToSend );
		}
		let initialSendTotal = std::min<uint8>( (uint8)ChunksToSend, threadSize );
		for( uint i=0; i<initialSendTotal; ++i )
			send( sp );
	}
}
namespace Jde{
	α IO::AioCompletionHandler( int /*signo*/, siginfo_t *info, void* /*context*/ )ι->void{
		// if( info->si_code != SI_AIO )
		// 	return;
		up<LinuxChunk> chunk{ (LinuxChunk*)info->si_value.sival_ptr };
		auto op = chunk->FileArg();
		if( let ec = aio_error64(&chunk->CB); ec ){
			op->PostExp( move(chunk), errno, Ƒ("AIO index: {} failed: {}\n", chunk->Index, strerror(errno)) );
			return;
		}
		ASSERT( aio_return64(&chunk->CB)==(_int)chunk->CB.aio_nbytes );
		if( op->ChunksToSend==++op->ChunksCompleted ){
			if( op->IsRead ){
				auto h = op->ReadCoHandle();
				Post( get<string>(move(op->Buffer)), move(h) ); //h.promise().Resume( get<string>(move(op->Buffer)), h );
				op->_coHandle = (TAwait<string>::Handle)nullptr;
			}
			else{
				Post( op->WriteCoHandle() );//get<VoidAwait::Handle>(arg.CoHandle).resume();
				op->_coHandle = (VoidAwait::Handle)nullptr;
			}
		}
		else{
			Post( [op](){IO::send(op);} );
		}
	}
}

/*
	α DriveAwaitable::await_resume()ι->AwaitResult{
		if( ExceptionPtr )
			return AwaitResult{ move(ExceptionPtr) };
		if( _cache && Cache::Has(_arg.Path) ){
			sp<void> pVoid = std::visit( [](auto&& x){return (sp<void>)x;}, _arg.Buffer );
			return AwaitResult{ move(pVoid) };
		}
		try{
			let size = _arg.Size();
			auto pData = std::visit( [](auto&& x){return x->data();}, _arg.Buffer );
			auto pEnd = pData+size;
			let chunkSize = DriveWorker::ChunkSize();
			let count = size/chunkSize+1;
			for( uint32 i=0; i<count; ++i ){
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


#include "WinFileAwait.h"
#include <boost/asio.hpp>
#include <boost\asio\post.hpp>
#include <jde/framework/Settings.h>
#include <jde/framework/str.h>
#include <jde/framework/io/FileAwait.h>
#include <jde/framework/process/thread.h>

#define let const auto

namespace Jde::IO{
	namespace asio = boost::asio;
	constexpr ELogTags _tags{ ELogTags::IO };
	up<asio::io_context> _ctx; mutex _ctxMutex;
	steady_clock::duration _keepAlive{ steady_clock::duration::zero() };
	Ω keepAlive()->steady_clock::duration{ return _keepAlive==steady_clock::duration::zero() ? Settings::FindDuration("/workers/io/keepAlive").value_or(5s) : _keepAlive; }

	atomic<uint> _threadCount;
	atomic<uint> _callCount;
	atomic<steady_clock::time_point> _finishFileIO{ steady_clock::time_point::min() };
	Ω keepAliveMillisecs()->std::chrono::milliseconds{
		let finishFileIO = _finishFileIO.load();
		auto milliSeconds = std::chrono::duration_cast<std::chrono::milliseconds>( finishFileIO==steady_clock::time_point::min() ? keepAlive() : steady_clock::now()-finishFileIO );
		return milliSeconds;
	}


	α FileIOArg::Open( bool create )ε->void{
		const DWORD access = IsRead ? GENERIC_READ : GENERIC_WRITE;
		const DWORD sharing = IsRead ? FILE_SHARE_READ : FILE_SHARE_WRITE;
		const DWORD creationDisposition = IsRead ? OPEN_EXISTING : !create &&fs::exists(Path) ? OPEN_EXISTING : CREATE_ALWAYS;
		const DWORD dwFlagsAndAttributes = IsRead ? FILE_FLAG_SEQUENTIAL_SCAN : FILE_ATTRIBUTE_ARCHIVE;
		auto tmp = Str::Replace( Path.string(), '/', '\\' );
		let path = string{"\\\\?\\"}+tmp;
		Handle = HandlePtr( WinHandle(::CreateFile(path.c_str(), access, sharing, nullptr, creationDisposition, FILE_FLAG_OVERLAPPED | dwFlagsAndAttributes, nullptr), [&](){
			return IOException(move(Path), GetLastError(), "CreateFile");
		}) );
		if( IsRead ){
			LARGE_INTEGER fileSize;
			THROW_IFX( !::GetFileSizeEx(Handle.get(), &fileSize), IOException(move(Path), GetLastError(), "GetFileSizeEx") );
			std::visit( [fileSize](auto&& b){b.resize(fileSize.QuadPart);}, Buffer );
		}
		TRACE( "[{}]{} size={}", Path.string().c_str(), IsRead ? "Read" : "Write", Size() );
	}

	Ω overlappedCompletionRoutine( DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED pOverlapped )->void;
	Ω send( FileIOArg& op )ι->void{
		up<IFileChunkArg> keepAlive;
		{
			lg l{ op.ChunkMutex };
			if( !op.Chunks.size() )
				return;
			keepAlive = move( op.Chunks.front() );
			op.Chunks.pop();
			Win::FileChunkArg& chunk = dynamic_cast<Win::FileChunkArg&>( *keepAlive.get() );
			if( op.IsRead ){
				if( !::ReadFileEx(op.Handle.get(), chunk.Buffer(), (DWORD)(chunk.Bytes), &chunk.Overlap, overlappedCompletionRoutine) )
					op.ResumeExp( GetLastError(), "Read failed", l );
			}
			else{
				TRACE( "({})Writing {} - {}", op.Path.string(), chunk.StartIndex, std::min(chunk.StartIndex+ChunkByteSize(), chunk.EndIndex) );
				if( !::WriteFileEx(op.Handle.get(), chunk.Buffer(), (DWORD)(chunk.Bytes), &chunk.Overlap, overlappedCompletionRoutine) )
					op.ResumeExp( GetLastError(), "Write Failed", l );
			}
		}
		for( int result = WAIT_TIMEOUT; result != WAIT_IO_COMPLETION; ){
			result = ::SleepEx( keepAliveMillisecs().count(), true );
			if( result==0 )
				WARN( "FileOp timed out." );
		}
	}

	α overlappedCompletionRoutine( DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED pOverlapped )->void{
		Win::FileChunkArg& chunk = *(Win::FileChunkArg*)pOverlapped->hEvent;
		auto arg = chunk.FileArg();
		if( dwErrorCode!=ERROR_SUCCESS )
			arg->ResumeExp( dwErrorCode, Ƒ("overlappedCompletionRoutine xfered='{}'", dwNumberOfBytesTransfered) );//no pOverlapped
		else if( arg->ChunksToSend==++arg->ChunksCompleted ){
			if( --_callCount==0 )
				_finishFileIO = steady_clock::now();
			if( arg->IsRead ){
				auto h = get<TAwait<string>::Handle>( arg->CoHandle() );
				h.promise().Resume( get<string>(move(arg->Buffer)), h );
			}
			else
				get<VoidAwait::Handle>(arg->CoHandle()).resume();
		}
		else{
			lg _{arg->ChunkMutex};
			if( arg->Chunks.size() )
				asio::post( *_ctx, [p=arg](){send( *p );} );
		}
	}

	α FileIOArg::Send( HCo h )ι->void{
		{
			lg l{ _coHandleMutex };
			_coHandle = h;
		}
		++_callCount;
		let threadSize = ThreadSize();
		{
			lg _{ _ctxMutex };
			if( !_ctx )
				_ctx = mu<asio::io_context>( threadSize );
		}
		let totalBytes = Size();
		let chunkByteSize = ChunkByteSize();
		lg l{ ChunkMutex };
		ChunksToSend = totalBytes/chunkByteSize+1;
		for( uint i=0; i*chunkByteSize<totalBytes; ++i )
			Chunks.emplace( mu<Win::FileChunkArg>(shared_from_this(), i) );
		TRACE( "[{}] chunks = {}", Path.string(), ChunksToSend );

		let initialSendTotal = std::min<uint8>( (uint8)ChunksToSend, threadSize );
		for( uint i=0; i<initialSendTotal; ++i ){
			asio::post( *_ctx, [this]{send( *this );} );
		}
		//let available = threadSize -  count_if(_threads, []( auto& t ){ return t.has_value(); }
		//let newThreadCount = std::min<uint8>( (uint8)chunkSize, threadSize );
		for( uint i = _threadCount.load(); i < initialSendTotal; ++i ){
			std::jthread{ [=]{
				SetThreadDscrptn( Ƒ("IO[{}]", i) );
				while( _callCount ){
					asio::steady_timer timer{ *_ctx };
					timer.expires_after( keepAlive() );
					_ctx->run();
				}
			}}.detach();
		}
	}
}
//#include <unistd.h>
#include <aio.h>
#include <liburing.h>
#include <jde/fwk/io/Cache.h>
#include <jde/fwk/io/file.h>
#include <jde/fwk/io/FileAwait.h>
#include <jde/fwk/process/execution.h>
#include <jde/fwk/process/thread.h>

#define let const auto
namespace Jde{
	struct io_uring _ring;
	α IO::LinuxInit()ι->void{
		io_uring_queue_init( 256, &_ring, 0 );
	}
}
namespace Jde::IO{
	constexpr ELogTags _tags{ ELogTags::IO };
	atomic<uint> _requestCount{};

	struct LinuxChunk final : IFileChunkArg{
		LinuxChunk( sp<FileIOArg> arg, uint index )ι:
			IFileChunkArg{ arg, index },
			StartIndex{ Index*ChunkByteSize() },
			EndIndex{ std::min(StartIndex+ChunkByteSize(), FileArg()->Size()) },
			Bytes{ EndIndex-StartIndex }
		{}
#undef StartIndex
		uint StartIndex;
		uint EndIndex;
		uint Bytes;
	};


	α FileIOArg::Open( bool create, bool append )ε->void{
		auto flags = O_NONBLOCK | ( IsRead ? O_RDONLY : O_WRONLY );
		if( !IsRead ){
			if( create )
				flags |= O_CREAT;
			if( append )
				 flags |= O_APPEND;
		}
		Handle = ::open( Path.string().c_str(), flags, 0666 );
		if( Handle==-1 ){
			Handle = 0;
			if( !IsRead && errno==ENOENT ){
				fs::create_directories( Path.parent_path() );
				INFO( "Created dir {}", Path.parent_path().string() );
				return Open( create, append );
			}
			THROW_IFX( IsRead /*|| errno!=EACCES*/, IOException(move(Path), errno, "open", _sl) );
		}
		if( IsRead ){
			struct stat st;
			THROW_IFX( ::fstat( Handle, &st )==-1, IOException(move(Path), errno, "fstat", _sl) );
			TRACE( "[{}]Opened file: {}, size: {}", hex(Handle), Path.string(), st.st_size );
			std::visit( [size=st.st_size](auto&& b){b.resize(size);}, Buffer );
		}
		else
			TRACE( "[{}]{} {}", hex(Handle), create ? "creating" : "appending", Path.string() );
	}
	FileIOArg::~FileIOArg(){
		if( Handle ){
			::close( Handle );
			TRACE( "[{}]Closed file handle for {}", hex(Handle), Path.string() );
			Handle = 0;
		}
	}

	Ω addNextChunkToQueue( sp<FileIOArg> op )ι->bool;
	uint _checkIndex;
	Ω processFinishedChunks( uint size, struct io_uring_cqe** cqe )ι->vector<sp<FileIOArg>>{
		vector<sp<FileIOArg>> submitOps;
		vector<sp<FileIOArg>> completedOps;
		for( uint i=0; i<size; ++i ){
			struct io_uring_cqe* cq = cqe[i];
			if( cq->flags & IORING_CQE_F_MORE ){
				bool buffered = cq->flags & IORING_CQE_F_BUFFER;
				bool nonEmpty = cq->flags & IORING_CQE_F_SOCK_NONEMPTY;
				let bufferId = (uint)io_uring_cqe_get_data(cq) >> 32;
				TRACE( "waitForMore data: {:x}, flags: {}, buffer: {}, nonEmpty: {}", bufferId, (uint)cq->flags, buffered, nonEmpty );
				continue;
			}
			io_uring_cqe_seen( &_ring, cq );
			up<LinuxChunk> chunk{ (LinuxChunk*)io_uring_cqe_get_data(cq) };
			ASSERT( chunk );
			if( !chunk )
				continue;
			if( cq->res < 0 ){
				auto op = chunk->FileArg();
				op->PostExp( move(chunk), -cq->res, Ƒ("AIO index: {} failed: {}\n", chunk->Index, strerror(-cq->res)) );
				continue;
			}
			ASSERT( (uint)cq->res == chunk->Bytes );
			auto op = chunk->FileArg();
			if( op->ChunksToSend>++op->ChunksCompleted ){
				if( addNextChunkToQueue(op) )
					submitOps.push_back(op);
			}
			else
				completedOps.push_back(op);
		}
		for( auto& completed : completedOps ){
			--_requestCount;
			if( completed->IsRead ){
				if( auto h = completed->ReadCoHandle(); h ){
					completed->_coHandle = (TAwait<string>::Handle)nullptr;
#ifdef __cpp_lib_move_only_function
					Post( get<string>(move(completed->Buffer)), move(h) );
#else
					auto p = new string{ get<string>(move(completed->Buffer)) };
					Post( [=](){
						h.promise().Resume( move(*p), h );
						delete p;
					} );
#endif
				}
			}
			else{
				if( auto h = completed->WriteCoHandle(); h ){
					completed->_coHandle = (VoidAwait::Handle)nullptr;
					Post( move(h) );
				}
				else
					CRITICAL( "[{}]no handle.", completed->Path.string() );
			}
		}
		return submitOps;
	}

	Ω checkProcessed( ELogTags _tags )ι->void;
	Ω submit( sp<FileIOArg> op, ELogTags _tags )ι->void{
		TRACE( "Submitting file IO: {}, size: {}, chunks: {}, requestCount: {}, isRead: {}", op->Path.string(), op->Size(), op->ChunksToSend, _requestCount.load(), op->IsRead );
		int result = io_uring_submit( &_ring );
		if( result>=0 ){
			ASSERT( _requestCount );
			if( _requestCount )
				PostIO( [tags=_tags](){ checkProcessed(tags); } );
		}
		else{
			CRITICAL( "io_uring_submit failed: {}", strerror(-result) );
			op->ResumeExp( -result, "io_uring_submit failed" );
		}
	}

	Ω checkProcessed( ELogTags _tags )ι->void{
		++_checkIndex;
		array<struct io_uring_cqe*, 5> cqe;
		let size = io_uring_peek_batch_cqe( &_ring, cqe.data(), cqe.size() );
		struct io_uring_cqe** pcqe = cqe.data();
		for( uint i=0; i<size; ++i ){
			struct io_uring_cqe* cq = pcqe[i];
			ASSERT( io_uring_cqe_get_data(cq) );
		}

		auto submitOps = size ? processFinishedChunks( size, pcqe ) : vector<sp<FileIOArg>>{};
		for( auto& op : submitOps )
			submit( move(op), _tags );
		if( submitOps.empty() && _requestCount )
			PostIO( [tags=_tags](){ checkProcessed(tags); } );
	}
	Ω addNextChunkToQueue( sp<FileIOArg> op )ι->bool{
		up<IFileChunkArg> chunk;
		{
			lg l{ op->ChunkMutex };
			if( !op->Chunks.size() ){
				TRACE( "No more chunks to queue for file IO: {}, size: {}, chunks: {}, requestCount: {}", op->Path.string(), op->Size(), op->ChunksToSend, _requestCount.load() );
				return false;
			}
			chunk = move( op->Chunks.front() );
			op->Chunks.pop();
		}
		let index = chunk->Index; let isRead = chunk->IsRead();
		LinuxChunk& lchunk = dynamic_cast<LinuxChunk&>( *chunk );
//		lg _{ _queueMutex };
		struct io_uring_sqe *sqe = io_uring_get_sqe( &_ring );
		sqe->flags |= IOSQE_IO_LINK;
		if( !sqe ){
			BREAK;
			auto message = Ƒ( "Could not get file queue:  aio_{} index: {}\n", isRead ? "read" : "write", index );
			lchunk.FileArg()->PostExp( move(chunk), errno, move(message) );
		}

		if( isRead ){
			TRACE( "Preparing read: {}, index: {}, bytes: {}", op->Path.string(), lchunk.Index, lchunk.Bytes );
			io_uring_prep_read( sqe, op->Handle, op->Data()+lchunk.StartIndex, lchunk.Bytes, lchunk.StartIndex );
		}
		else{
			TRACE( "Preparing write: {}, index: {}, bytes: {}", op->Path.string(), lchunk.Index, lchunk.Bytes );
			io_uring_prep_write( sqe, op->Handle, op->Data()+lchunk.StartIndex, lchunk.Bytes, -1 );
		}
		auto pChunk = chunk.release();
		io_uring_sqe_set_data( sqe, pChunk );
		return true;
	}
	uint fileIndex{};
	α FileIOArg::Send( HCo h )ι->void{
		_coHandle = h;
		let threadSize = ThreadSize();
		let totalBytes = Size();
		let chunkByteSize = ChunkByteSize();
		auto self = shared_from_this();
		{
			//lg l{ ChunkMutex };
			ChunksToSend = (totalBytes+chunkByteSize-1)/chunkByteSize; //ceil( totalBytes / chunkByteSize )
			for( uint i=0; i*chunkByteSize<totalBytes; ++i )
				Chunks.emplace( mu<LinuxChunk>(self, i) );
			let content = IsRead ? "" : Str::Replace(string{ Data(), Size() }, "\n", "\\n" );
		}
		++_requestCount;
		PostIO( [self, initialSendTotal = std::min<uint8>((uint8)ChunksToSend, threadSize), tags=_tags ](){
			for( uint i=0; i<initialSendTotal; ++i )
				addNextChunkToQueue( self );
			submit( move(self), tags );
		} );
	}
}
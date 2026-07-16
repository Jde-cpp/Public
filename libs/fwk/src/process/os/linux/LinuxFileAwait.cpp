//#include <unistd.h>
#include <aio.h>
#include <liburing.h>
#include <sys/eventfd.h>
#include <boost/asio.hpp>
#include <jde/fwk/io/Cache.h>
#include <jde/fwk/io/file.h>
#include <jde/fwk/io/FileAwait.h>
#include <jde/fwk/process/execution.h>
#include <jde/fwk/process/thread.h>

#define let const auto
namespace Jde{
	struct io_uring _ring;
	int _eventFd{ -1 };//signaled by the kernel when a cqe posts; -1 → fall back to polling.
	α IO::LinuxInit()ι->void{
		io_uring_queue_init( 256, &_ring, 0 );
		_eventFd = ::eventfd( 0, EFD_CLOEXEC | EFD_NONBLOCK );
		if( _eventFd==-1 || io_uring_register_eventfd(&_ring, _eventFd)<0 ){
			WARNT( ELogTags::IO, "eventfd registration failed - file io will poll for completions. errno: {}", errno );
			if( _eventFd!=-1 ){
				::close( _eventFd );
				_eventFd = -1;
			}
		}
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
		for( bool retried = false;; retried = true ){
			Handle = ::open( Path.string().c_str(), flags, 0666 );
			if( Handle!=-1 )
				break;
			Handle = 0;
			let err = errno;
			if( !retried && !IsRead && err==ENOENT ){//parent dir may not exist - create it & retry once.
				std::error_code ec;
				fs::create_directories( Path.parent_path(), ec );
				THROW_IFX( ec, IOException(move(Path), (uint32)ec.value(), "create_directories", _sl) );
				INFO( "Created dir {}", Path.parent_path().string() );
				continue;
			}
			throw IOException{ move(Path), (uint32)err, "open", _sl };
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

	Ω prepChunk( up<IFileChunkArg>&& chunk, const sp<FileIOArg>& op )ι->bool{
		let index = chunk->Index; let isRead = chunk->IsRead();
		LinuxChunk& lchunk = dynamic_cast<LinuxChunk&>( *chunk );
		struct io_uring_sqe *sqe = io_uring_get_sqe( &_ring );
		if( !sqe ){
			BREAK;
			auto message = Ƒ( "Could not get file queue:  aio_{} index: {}\n", isRead ? "read" : "write", index );
			op->PostExp( move(chunk), EBUSY, move(message) );
			return false;
		}
		//no IOSQE_IO_LINK: a link chain spans whatever sqes share a submission batch - including unrelated
		//ops/files - and a short read severs the chain, canceling the rest with -ECANCELED. Ordering isn't
		//needed here: reads use explicit offsets and appends allow only one write chunk in flight (Send).

		if( isRead ){
			TRACE( "Preparing read: {}, index: {}, bytes: {}", op->Path.string(), lchunk.Index, lchunk.Bytes );
			io_uring_prep_read( sqe, op->Handle, op->Data()+lchunk.StartIndex, lchunk.Bytes, lchunk.StartIndex );
		}
		else{
			TRACE( "Preparing write: {}, index: {}, bytes: {}", op->Path.string(), lchunk.Index, lchunk.Bytes );
			io_uring_prep_write( sqe, op->Handle, op->Data()+lchunk.StartIndex, lchunk.Bytes, -1 );
		}
		io_uring_sqe_set_data( sqe, chunk.release() );
		return true;
	}

	Ω addNextChunkToQueue( sp<FileIOArg> op )ι->bool;
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
			let res = cq->res;
			up<LinuxChunk> chunk{ (LinuxChunk*)io_uring_cqe_get_data(cq) };
			io_uring_cqe_seen( &_ring, cq );//releases the slot for kernel reuse - cq must not be read past this point.
			ASSERT( chunk );
			if( !chunk )
				continue;
			if( res < 0 ){
				auto op = chunk->FileArg();
				op->PostExp( move(chunk), -res, Ƒ("AIO index: {} failed: {}\n", chunk->Index, strerror(-res)) );
				continue;
			}
			if( (uint)res < chunk->Bytes ){//partial read/write - resubmit the remainder.
				auto op = chunk->FileArg();
				if( res==0 ){//no progress - EOF or full disk; resubmitting would loop forever.
					op->PostExp( move(chunk), EIO, Ƒ("AIO index: {} {} returned 0 with {} bytes remaining.\n", chunk->Index, chunk->IsRead() ? "read" : "write", chunk->Bytes) );
					continue;
				}
				TRACE( "Partial {}: {}, index: {}, completed: {} of {} - resubmitting remainder.", chunk->IsRead() ? "read" : "write", op->Path.string(), chunk->Index, res, chunk->Bytes );
				chunk->StartIndex += res;
				chunk->Bytes -= res;
				if( prepChunk(move(chunk), op) )
					submitOps.push_back( op );
				continue;
			}
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

	Ω armCompletionWait( ELogTags tags )ι->void;
	Ω submit( sp<FileIOArg> op, ELogTags _tags )ι->void{
		TRACE( "Submitting file IO: {}, size: {}, chunks: {}, requestCount: {}, isRead: {}", op->Path.string(), op->Size(), op->ChunksToSend, _requestCount.load(), op->IsRead );
		int result = io_uring_submit( &_ring );
		if( result>=0 ){
			ASSERT( _requestCount );
			if( _requestCount )
				armCompletionWait( _tags );
		}
		else{
			CRITICAL( "io_uring_submit failed: {}", strerror(-result) );
			op->ResumeExp( -result, "io_uring_submit failed" );
		}
	}

	Ω drainCompletions( ELogTags _tags )ι->void{
		array<struct io_uring_cqe*, 16> cqe;
		for(;;){
			let size = io_uring_peek_batch_cqe( &_ring, cqe.data(), cqe.size() );
			if( !size )
				break;
			auto submitOps = processFinishedChunks( size, cqe.data() );
			for( auto& op : submitOps )
				submit( move(op), _tags );
		}
		if( _requestCount )
			armCompletionWait( _tags );
	}

	up<boost::asio::posix::stream_descriptor> _eventSd;
	uint64_t _eventCount;
	bool _armed{};//io-strand confined
	struct EventFdShutdown final : IShutdown{//descriptor must die before the io_context does.
		α Shutdown( bool /*terminate*/, SL )ι->void override{
			if( _eventSd ){
				_eventSd->release();//keeps the eventfd open & registered with the ring in case the executor restarts.
				_eventSd = nullptr;
				_armed = false;
			}
		}
	};
	EventFdShutdown _eventSdShutdown;

	Ω armCompletionWait( ELogTags tags )ι->void{
		if( _eventFd==-1 ){//no eventfd - fall back to polling.
			PostIO( [tags](){ drainCompletions(tags); } );
			return;
		}
		if( _armed )
			return;
		if( !_eventSd ){
			_eventSd = mu<boost::asio::posix::stream_descriptor>( *Executor(), _eventFd );
			Execution::AddShutdown( &_eventSdShutdown );
		}
		_armed = true;
		boost::asio::async_read( *_eventSd, boost::asio::buffer(&_eventCount, sizeof(_eventCount)),
			[tags]( const boost::system::error_code& ec, size_t ){
				if( ec )
					return;//cancelled at shutdown
				PostIO( [tags](){
					_armed = false;
					drainCompletions( tags );
				} );
			} );
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
		return prepChunk( move(chunk), op );
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
		}
		if( ChunksToSend==0 ){//empty file - no completions will arrive; resume immediately.
			TRACE( "[{}]Empty file - resuming without io.", Path.string() );
			if( IsRead ){
				if( auto h2 = ReadCoHandle(); h2 ){
#ifdef __cpp_lib_move_only_function
					Post( get<string>(move(Buffer)), move(h2) );
#else
					auto p = new string{ get<string>(move(Buffer)) };
					Post( [=](){
						h2.promise().Resume( move(*p), h2 );
						delete p;
					} );
#endif
				}
			}
			else if( auto h2 = WriteCoHandle(); h2 )
				Post( move(h2) );
			return;
		}
		++_requestCount;
		//writes append (offset -1/O_APPEND): concurrent in-flight chunks can be executed out of order by the
		//kernel (io-wq), permuting the file's contents - only one write chunk may be in flight at a time.
		//reads use explicit offsets, so a window of parallel chunks is safe.
		PostIO( [self, initialSendTotal = IsRead ? std::min<uint>(ChunksToSend, threadSize) : 1u, tags=_tags ](){
			for( uint i=0; i<initialSendTotal; ++i )
				addNextChunkToQueue( self );
			submit( move(self), tags );
		} );
	}
}
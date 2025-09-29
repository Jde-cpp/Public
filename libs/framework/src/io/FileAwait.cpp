#include <jde/framework/io/FileAwait.h>
#include <jde/framework/process/execution.h>
#include <jde/framework/io/Cache.h>

#define let const auto
namespace Jde{
	uint32 _chunkSize;
	α IO::ChunkByteSize()ι->uint32{ return _chunkSize; }
	uint8 _threadSize;
	α IO::ThreadSize()ι->uint8{ return _threadSize; }
	α IO::Init()ι->void{
		_chunkSize = Settings::FindNumber<uint32>("/workers/io/chunkByteSize").value_or(1 << 19);
		_threadSize = Settings::FindNumber<uint8>("/workers/io/threads").value_or(5);
#ifndef _MSC_VER
		LinuxInit();
#endif
	}

namespace IO{
	constexpr ELogTags _tags = ELogTags::IO;
	α IFileChunkArg::Handle()Ι->HFile&{ return _fileIOArg->Handle; }
	α IFileChunkArg::IsRead()Ι->bool{ return _fileIOArg->IsRead; }

	FileIOArg::FileIOArg( fs::path path, bool vec, SL sl )ι:
		IsRead{ true },
		Path{ move(path) },
		_sl{ sl }{
		if( vec )
			Buffer = vector<byte>{};
	}
	FileIOArg::FileIOArg( fs::path path, variant<string,vector<byte>> data, ELogTags tags, SL sl )ι:
		Buffer{ move(data) },
		IsRead{ false },
		Path{ move(path) },
		_sl{ sl },
		_tags{ tags }
	{}
/*
	bool FileIOArg::HandleChunkComplete( IFileChunkArg* pChunkArg )ι{
		ul l{ Chunks.Mutex };
		IFileChunkArg* pNextChunk{ nullptr };
		up<IFileChunkArg> pChunk;
		bool additional{ false };
		for( auto pp=Chunks.begin(l); !(pChunk && pNextChunk) && pp!=Chunks.end(l); ++pp )
		{
			if( *pp==nullptr )
				continue;
			if( (*pp).get()==pChunkArg )
				pChunk = move( *pp );
			else if( !pNextChunk && !(*pp)->Sent.exchange(true) )
				pNextChunk = (*pp).get();
			else
				additional = true;
		}
		ASSERT( pChunk );
		if( pNextChunk )
			pNextChunk->Process();
		else
		{
			//DBG( "[{}] close"sv, pChunkArg->Handle() );
			//::close( pChunkArg->Handle() );//TODO fix this on windows.
		}
		return !pNextChunk && !additional;
	}
*/

	α FileIOArg::PostExp( up<IFileChunkArg>&& chunk, uint32 code, string&& m )ι->void{
		{
			lg l{ ChunkMutex };
			while( Chunks.size() )
				Chunks.pop();
		}
		if( IsRead ){
			if( auto h = ReadCoHandle(); h ){
				_coHandle = (TAwait<string>::Handle)nullptr;
				Post( [path=move(Path),sl=_sl, m=move(m), code, h](){h.promise().ResumeExp(IOException{path, code, move(m), sl}, h);} );
			}
		}
		else{
			if( auto h = WriteCoHandle(); h ){
				_coHandle = (VoidAwait::Handle)nullptr;
				Post( [path=move(Path),sl=_sl, m=move(m), code, h](){h.promise().ResumeExp(IOException{path, code, move(m), sl}, h);} );
			}
		}
		chunk=nullptr;
	}

	α FileIOArg::ResumeExp( uint32 code, string&& m )ι->void{
		lg l{ ChunkMutex };
		ResumeExp( code, move(m), l );
	}
	α FileIOArg::ResumeExp( uint32 code, string&& m, lg& /*chunkLock*/ )ι->void{
		IOException e{ Path, code, move(m), _sl };
		if( IsRead ){
			auto h = ReadCoHandle();
			h.promise().ResumeExp( move(e), h );
		}
		else{
			auto h = WriteCoHandle();
			h.promise().ResumeExp( move(e), h );
		}
	}

/*	α FileIOArg::ResumeExp( exception&& e )ι->void{
		lg l{ ChunkMutex };
		ResumeExp( move(e), l );
	}
	α FileIOArg::ResumeExp( exception&& e, lg& / *chunkLock* / )ι->void{
		while( Chunks.size() )
			Chunks.pop();
		if( IsRead ){
			auto h = get<TAwait<string>::Handle>( CoHandle );
			if( h ){
				CoHandle = (TAwait<string>::Handle)nullptr;
				h.promise().ResumeExp( move(e), h );
			}
		}
		else{
			auto h = get<VoidAwait::Handle>( CoHandle );
			if( h ){
				CoHandle = (VoidAwait::Handle)nullptr;
				h.promise().ResumeExp( move(e), h );
			}
		}
	}
*/
	// α FileIOArg::Send( coroutine_handle<Task2::promise_type>&& h )ι->void
	// {
	// 	CoHandle = move( h );
	// 	for( uint i=0; i*DriveWorker::ChunkSize()<Size(); ++i )
	// 		Chunks.emplace_back( CreateChunk(i) );
	// 	OSSend();
	// }

	α ReadAwait::await_ready()ι->bool{
		if( auto p = _cache ? Cache::Get<string>(_arg->Path.string()) : sp<string>{}; p ){
			_arg->Buffer = *p;
			return true;
		}
		else{
			try{
				_arg->Open( false );
			}
			catch( IOException& e ){
				ExceptionPtr = e.Move();
			}
		}
		return ExceptionPtr!=nullptr;
	}

	α WriteAwait::await_ready()ι->bool{
		try{
			_arg->Open( _create );
		}
		catch( IOException& e ){
			ExceptionPtr = e.Move();
		}
		return ExceptionPtr!=nullptr;
	}

	α ReadAwait::Suspend()ι->void{
		_arg->Send( _h );
	}
	α WriteAwait::Suspend()ι->void{
		_arg->Send( _h );
	}
	α ReadAwait::await_resume()ε->string{
		if( ExceptionPtr )
			ExceptionPtr->Throw();
		auto& r = get<string>(_arg->Buffer);
		if( r.size() )
			return move(r);
		auto y = TAwait<string>::await_resume();
		if( _cache )
			Cache::Set<string>( _arg->Path.string(), ms<string>(y) );
		return y;
	}
	α WriteAwait::await_resume()ε->void{
		if( ExceptionPtr )
			ExceptionPtr->Throw();
		VoidAwait::await_resume();
	}
}}
#include <jde/fwk/io/FileAwait.h>
#include <jde/fwk/process/execution.h>
#include <jde/fwk/io/Cache.h>

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

	α FileIOArg::PostExp( up<IFileChunkArg>&& chunk, uint32 code, string&& m )ι->void{
		{
			lg l{ ChunkMutex };
			while( Chunks.size() )
				Chunks.pop();
		}
		if( IsRead ){
			if( auto h = ReadCoHandle(); h ){
				_coHandle = (StringAwait::Handle)nullptr;
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
		auto y = StringAwait::await_resume();
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
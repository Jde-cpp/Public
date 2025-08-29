#pragma once
#ifndef FILECO_H
#define FILECO_H
#include <queue>
#include <jde/framework/coroutine/Task.h>
#ifdef _MSC_VER
	#include <jde/framework/process/os/windows/WindowsHandle.h>
	using HFile=Jde::HandlePtr;
#else
	using HFile=int;
#endif
#include <jde/framework/coroutine/Await.h>

namespace Jde::IO{
	struct FileIOArg;

	Γ α ChunkByteSize()ι->uint32;
	Γ α ThreadSize()ι->uint8;

	struct IFileChunkArg{
		IFileChunkArg( FileIOArg& arg, uint index ):
			Index{index},
			_fileIOArg{arg}
		{}
		virtual ~IFileChunkArg()=default;
		β Handle()ι->HFile&;
		β Process()ι->void{};
		β FileArg()ι->FileIOArg&{ return _fileIOArg;}
		β FileArg()Ι->const FileIOArg&{ return _fileIOArg;}

		std::atomic<bool> Sent;
		uint Index;
		FileIOArg& _fileIOArg;
	};

	struct FileIOArg final{
		using HCo=variant<TAwait<string>::Handle,VoidAwait::Handle>;
		FileIOArg( fs::path path, bool vec, SRCE )ι;
		FileIOArg( fs::path path, variant<string,vector<char>> data, SRCE )ι;
		α Open( bool create )ε->void;
		α Send( HCo h )ι->void;
		α Data()ι{ return visit( [](auto&& x){return x.data();}, Buffer ); }
		α Size()Ι{ return visit( [](auto&& x){return x.size();}, Buffer ); }
		α ResumeExp( exception&& e )ι->void;
		α ResumeExp( exception&& e, lg& chunkLock )ι->void;

		variant<string,vector<char>> Buffer;
		std::queue<up<IFileChunkArg>> Chunks; std::mutex ChunkMutex;
		atomic<uint> ChunksCompleted;
		uint ChunksToSend;
		HCo CoHandle{};
		HFile Handle{};
		bool IsRead;
		fs::path Path;
		SL _sl;
	};

	struct Γ IFileAwait{
		IFileAwait( fs::path&& path, bool vec, SL sl )ι:_arg{ move(path), vec, sl }{}
		IFileAwait( fs::path&& path, variant<string,vector<char>>&& data, SL sl )ι:_arg{ move(path), move(data), sl }{}
		up<IException> ExceptionPtr;
		FileIOArg _arg;
	};
	struct Γ ReadAwait final : IFileAwait, TAwait<string>{
		ReadAwait( fs::path path, bool cache, SRCE )ι:IFileAwait{ move(path), false, sl }, TAwait<string>{ sl },_cache{cache}{}
		α Suspend()ι->void override;
		α await_ready()ι->bool override;
		α await_resume()ε->string override;
	private:
		bool _cache;
	};
	struct Γ WriteAwait final : IFileAwait, VoidAwait, boost::noncopyable{
		WriteAwait( fs::path path, variant<string,vector<char>> data, SRCE )ι:WriteAwait{ move(path), move(data), false, sl }{}
		WriteAwait( fs::path path, variant<string,vector<char>> data, bool create, SRCE )ι:IFileAwait{ move(path), move(data), sl }, VoidAwait{ sl }, _create{create}{}
		α Suspend()ι->void override;
		α await_ready()ι->bool override;
		α await_resume()ε->void override;
	private:
		bool _create;
	};
}
#endif
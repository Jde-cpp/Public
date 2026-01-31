#pragma once
#ifndef FILECO_H
#define FILECO_H
#include <queue>
#include <jde/fwk/co/Task.h>
#ifdef _MSC_VER
	#include <jde/fwk/process/os/windows/WindowsHandle.h>
	using HFile=Jde::HandlePtr;
#else
	using HFile=int;
#endif
#include <jde/fwk/co/Await.h>

#define Φ Γ auto

namespace Jde::IO{
	Φ Init()ι->void;
#ifndef _MSC_VER
	α LinuxInit()ι->void;
#endif

	Φ ChunkByteSize()ι->uint32;
	Φ ThreadSize()ι->uint8;

	struct FileIOArg;
	struct IFileChunkArg{
		IFileChunkArg( sp<FileIOArg>& arg, uint index ):
			Index{index},
			_fileIOArg{arg}
		{}
		virtual ~IFileChunkArg()=default;
		β Handle()Ι->HFile&;
		β Process()ι->void{};
		β FileArg()Ι->sp<FileIOArg>{ return _fileIOArg; }
		α IsRead()Ι->bool;

		std::atomic<bool> Sent;
		uint Index;
	private:
		sp<FileIOArg> _fileIOArg;
	};

	struct FileIOArg final : std::enable_shared_from_this<FileIOArg>, noncopyable{
		using HCo=variant<StringAwait::Handle,VoidAwait::Handle>;
		FileIOArg( fs::path path, bool vec, SRCE )ι;
		FileIOArg( fs::path path, variant<string,vector<byte>> data, ELogTags tags, SRCE )ι;
		~FileIOArg();
		α Open( bool create )ε->void;
		α Send( HCo h )ι->void;
		α Data()ι->char*{ return visit( [](auto&& x){return (char*)x.data();}, Buffer ); }
		α Size()Ι{ return visit( [](auto&& x){return x.size();}, Buffer ); }
		α PostExp( up<IFileChunkArg>&& chunk, uint32 code, string&& msg )ι->void;
		α ResumeExp( uint32 code, string&& msg )ι->void;
		α ResumeExp( uint32 code, string&& m, lg& chunkLock )ι->void;
		α Key()Ι->uint8{ return std::hash<fs::path>{}( Path ); }

		//α ResumeExp( exception&& e )ι->void;
		//α ResumeExp( exception&& e, lg& chunkLock )ι->void;

		α CoHandle()ι->HCo{ lg _{_coHandleMutex}; auto h = _coHandle; if( IsRead ) _coHandle = StringAwait::Handle{}; else _coHandle = VoidAwait::Handle{}; return h; }
		α ReadCoHandle()ι->StringAwait::Handle{ return get<StringAwait::Handle>(CoHandle()); }
		α WriteCoHandle()ι->VoidAwait::Handle{ return get<VoidAwait::Handle>(CoHandle()); }

		variant<string,vector<byte>> Buffer;
		std::queue<up<IFileChunkArg>> Chunks; std::mutex ChunkMutex;
		atomic<uint> ChunksCompleted;
		uint ChunksToSend;
		HFile Handle{};
		bool IsRead;
		fs::path Path;
		SL _sl;
		ELogTags _tags;
		HCo _coHandle; mutex _coHandleMutex;
	};

	struct Γ IFileAwait{
		IFileAwait( fs::path&& path, bool vec, SL sl )ι:_arg{ ms<FileIOArg>(move(path), vec, sl) }{}
		IFileAwait( fs::path&& path, variant<string,vector<byte>>&& data, ELogTags tags, SL sl )ι:_arg{ ms<FileIOArg>(move(path), move(data), tags, sl) }{}
		up<IException> ExceptionPtr;
		sp<FileIOArg> _arg;
	};
	struct Γ ReadAwait final : IFileAwait, StringAwait, noncopyable{
		ReadAwait( fs::path path, bool cache=false, SRCE )ι:IFileAwait{ move(path), false, sl }, StringAwait{ sl },_cache{cache}{}
		α Suspend()ι->void override;
		α await_ready()ι->bool override;
		α await_resume()ε->string override;
	private:
		bool _cache;
	};
	struct Γ WriteAwait final : IFileAwait, VoidAwait, noncopyable{
//		WriteAwait( fs::path path, variant<string,vector<char>> data, SRCE )ι:WriteAwait{ move(path), move(data), false, sl }{}
		WriteAwait( fs::path path, variant<string,vector<byte>> data, bool create=false, ELogTags tags=ELogTags::IO, SRCE )ι:
			IFileAwait{ move(path), move(data), tags, sl }, VoidAwait{ sl }, _create{create}{}
		α Suspend()ι->void override;
		α await_ready()ι->bool override;
		α await_resume()ε->void override;
	private:
		bool _create;
	};
}
#undef Φ
#endif
#include <jde/framework/io/FileAwait.h>

namespace Jde::IO::Win{
	struct FileChunkArg : IFileChunkArg{
		FileChunkArg( FileIOArg& arg, uint index ):
			IFileChunkArg{ arg, index },
			StartIndex{ Index*ChunkByteSize() },
			EndIndex{ std::min(StartIndex+ChunkByteSize(), FileArg().Size()) },
			Overlap{ .Pointer=(PVOID)(Index==0 && !arg.IsRead ? -1 : StartIndex), .hEvent=this },
			Bytes{ EndIndex-StartIndex }
		{}

		α Buffer()->char*{ return FileArg().Data()+StartIndex; }
		uint StartIndex;
		uint EndIndex;
		::OVERLAPPED Overlap;
		uint Bytes;
	};
}
#include <fstream>
#include <jde/fwk/io/Cache.h>
#include <jde/fwk/io/FileAwait.h>
#include <jde/fwk/co/LockKey.h>
#include <jde/fwk/process/execution.h>
#include <jde/fwk/utils/Vector.h>
#include <jde/fwk/process/thread.h>

#define let const auto

using boost::uuids::uuid;
namespace Jde::IO::Tests{
	constexpr ELogTags _tags{ ELogTags::Test };
	Ω file( uint index )->fs::path{
		let path = Settings::FindPath( "/testing/file" );
		return path
			? path->parent_path()/Ƒ( "{}{}{}", path->stem().string(), index, path->extension().string() )
			: fs::temp_directory_path()/Ƒ( "test{}.txt", index );
	}

	struct FileTests : public ::testing::Test{
	protected:
		FileTests(){}
		~FileTests()override{}

		Ω SetUpTestCase()ι->void{
			Execution::Run();//io strand threading issue.
		}
		α SetUp()->void override{
			INFO( "{}", file(0).string() );
			fs::create_directories( file(0).parent_path() );
		}
		α TearDown()->void override{}
	};

	Ω write( const fs::path& file, uuid guid1, uuid guid2, Vector<uuid>& written, bool createFile, SRCE )->LockKeyAwait::Task{
		auto l = co_await LockKeyAwait{ file.string() };
		[sl]( const fs::path& file, uuid guid1, uuid guid2, Vector<uuid>& written, CoLockGuard, bool createFile )->VoidAwait::Task {
			try{
				co_await IO::WriteAwait{ file, Ƒ("{}\n{}\n", ToString(guid1), ToString(guid2)), createFile, Jde::ELogTags::IO, sl };
			}
			catch( Exception& e ){
				e.Log();
				e.Throw();
			}
			written.push_back( guid1 );
			written.push_back( guid2 );
		}( file, guid1, guid2, written, move(l), createFile );
	}
	Ω read( const fs::path& file, Vector<uuid>& readValues, SRCE )ι->TAwait<string>::Task{
		try{
			let content = co_await IO::ReadAwait{ file, false, sl };
			let guidStrings = Str::Split( content, '\n' );
			ul l{ readValues.Mutex };
			for( auto&& guid : guidStrings ){
				try{
					readValues.push_back( ToUuid(string{guid}), l );
				}
				catch( const std::exception& e ){
					THROW( "[{}] Failed to parse GUID from string '{}': {}", file.string(), guid, e.what() );
				}
			}
		}
		catch( const std::exception& e ){
			throw e;
		}
	}

	Ω testFile( uint fileIndex )->void{
		array<uuid,1024> guids;
		for( uint i=0; i<guids.size(); ++i ){
			auto prefix = boost::endian::endian_reverse( fileIndex );
			( (uint*)guids[i].data() )[0] = prefix;
			auto suffix = boost::endian::endian_reverse( i );
			( (uint*)guids[i].data() )[1] = suffix;
		}

		Vector<uuid> written;
		let file = Tests::file( fileIndex );
		let exists = fs::exists( file );
		if( exists ){
			INFO( "Removing existing file: {}", file.string() );
			fs::remove( file );
		}
		Thread::SetName( file.filename().string() );
		for( uint i=0; i<guids.size(); i+=2 )
			write( file, guids[i], guids[i+1], written, i==0 );

		while( written.size()<guids.size() )
			std::this_thread::sleep_for( 10ms );

		Vector<uuid> readValues;
		read( file, readValues );
		while( readValues.empty() )
			std::this_thread::sleep_for( 10ms );

		ASSERT_TRUE( readValues.size() == guids.size() );
		readValues.visit( [&](const uuid& guid){
			ASSERT_TRUE( find(guids, guid)!=guids.end() );
		});
	}

	Ω writeRaw( fs::path file, string content, std::atomic<bool>& done, SRCE )->LockKeyAwait::Task{
		auto l = co_await LockKeyAwait{ file.string() };
		[sl]( fs::path file, string content, std::atomic<bool>& done, CoLockGuard )->VoidAwait::Task {
			try{
				co_await IO::WriteAwait{ move(file), move(content), true, Jde::ELogTags::IO, sl };
			}
			catch( Exception& e ){
				e.Log();
			}
			done = true;
		}( move(file), move(content), done, move(l) );
	}

	Ω readRaw( fs::path file, sp<string> content, std::atomic<bool>& done, bool cache=false, SRCE )ι->TAwait<string>::Task{
		try{
			*content = co_await IO::ReadAwait{ move(file), cache, sl };
		}
		catch( Exception& e ){
			e.Log();
		}
		done = true;
	}

	// Regression for the ChunksToSend off-by-one: a write whose size is an exact multiple of
	// ChunkByteSize() must still complete. ChunksToSend was computed as size/ChunkByteSize()+1,
	// one more than the number of chunks actually queued whenever the size divides evenly, so the
	// final chunk's completion never satisfied ChunksToSend==ChunksCompleted and the write hung.
	// 256 guards the Windows initial-window truncation: (uint8)ChunksToSend turned any multiple of
	// 256 chunks into a zero-size window, so no chunk ever started and the coroutine hung.
	TEST_F( FileTests, WriteExactChunkMultiple ){
		let chunkSize = IO::ChunkByteSize();
		for( uint chunks : {1u, 2u, 3u, 256u} ){
			let size = chunks*chunkSize;
			let file = Tests::file( 100+chunks );
			if( fs::exists(file) )
				fs::remove( file );
			let content = string( size, (char)('A'+chunks) ); //distinct, deterministic per file
			std::atomic<bool> done{};
			writeRaw( file, content, done );

			let deadline = steady_clock::now()+10s;
			while( !done && steady_clock::now()<deadline )
				std::this_thread::sleep_for( 5ms );
			ASSERT_TRUE( done ) << "write of " << size << " bytes (" << chunks << " chunk(s)) never completed";

			std::ifstream is{ file, std::ios::binary };
			let actual = string{ std::istreambuf_iterator<char>{is}, std::istreambuf_iterator<char>{} };
			ASSERT_EQ( actual.size(), size );
			ASSERT_EQ( actual, content );
		}
	}

	// Regression: a zero-byte operation produced no chunks, so no completion ever arrived — the
	// awaiting coroutine hung and a completion poller was leaked. Empty writes must create the file
	// and resume; reads of empty files must resume with an empty string.
	TEST_F( FileTests, EmptyFile ){
		let file = Tests::file( 200 );
		if( fs::exists(file) )
			fs::remove( file );

		std::atomic<bool> written{};
		writeRaw( file, {}, written );
		let writeDeadline = steady_clock::now()+10s;
		while( !written && steady_clock::now()<writeDeadline )
			std::this_thread::sleep_for( 5ms );
		ASSERT_TRUE( written ) << "empty write never completed";
		ASSERT_TRUE( fs::exists(file) );
		ASSERT_EQ( fs::file_size(file), 0u );

		auto content = ms<string>( "sentinel" );
		std::atomic<bool> readDone{};
		readRaw( file, content, readDone );
		let readDeadline = steady_clock::now()+10s;
		while( !readDone && steady_clock::now()<readDeadline )
			std::this_thread::sleep_for( 5ms );
		ASSERT_TRUE( readDone ) << "empty read never completed";
		ASSERT_TRUE( content->empty() ) << "expected empty content, got: " << *content;
	}

	TEST_F( FileTests, CachedEmptyString ){
		let file = Tests::file( 300 );//never created on disk - a cache hit must not open the file.
		Cache::Set<string>( file.string(), "" );//explicit - `{}` is ambiguous between the T and sp<const T> overloads.
		auto content = ms<string>( "sentinel" );
		std::atomic<bool> done{};
		readRaw( file, content, done, true );
		let deadline = steady_clock::now()+10s;
		while( !done && steady_clock::now()<deadline )
			std::this_thread::sleep_for( 5ms );
		ASSERT_TRUE( done ) << "cached empty read never completed";
		EXPECT_TRUE( content->empty() ) << "expected empty content, got: " << *content;
		Cache::Clear( file.string() );
	}

	constexpr uint _fileSize{ 5 };
	TEST_F( FileTests, WriteRead ){
		ASSERT_TRUE( IO::ChunkByteSize()<74 ); //guid+\n*2
		ASSERT_TRUE( IO::ThreadSize()>1 ); //guid+\n
		vector<std::jthread> threads;
		for( uint i=0; i<_fileSize; ++i )
			threads.emplace_back( [i](){testFile(i);} );
		for( auto& thread : threads )
				thread.join();
	}
}
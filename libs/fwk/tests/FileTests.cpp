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

	//done is co-owned: on a timed-out wait the test returns while the detached coroutine is still running, so a
	//reference would dangle when the completion finally stores true.
	Ω writeRaw( fs::path file, string content, sp<std::atomic<bool>> done, SRCE )->LockKeyAwait::Task{
		auto l = co_await LockKeyAwait{ file.string() };
		[sl]( fs::path file, string content, sp<std::atomic<bool>> done, CoLockGuard )->VoidAwait::Task {
			try{
				co_await IO::WriteAwait{ move(file), move(content), true, Jde::ELogTags::IO, sl };
			}
			catch( Exception& e ){
				e.Log();
			}
			*done = true;
		}( move(file), move(content), move(done), move(l) );
	}

	Ω readRaw( fs::path file, sp<string> content, sp<std::atomic<bool>> done, bool cache=false, SRCE )ι->TAwait<string>::Task{
		try{
			*content = co_await IO::ReadAwait{ move(file), cache, sl };
		}
		catch( Exception& e ){
			e.Log();
		}
		*done = true;
	}

	//poll until the co-owned flag flips or the 10s deadline passes; returns whether it completed.  The ASSERT stays
	//at the call site so a GoogleTest fatal failure returns from the test, not just from here.
	Ω waitDone( const std::atomic<bool>& done )ι->bool{
		let deadline = steady_clock::now()+10s;
		while( !done && steady_clock::now()<deadline )
			std::this_thread::sleep_for( 5ms );
		return done;
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
			auto done = ms<std::atomic<bool>>();
			writeRaw( file, content, done );
			ASSERT_TRUE( waitDone(*done) ) << "write of " << size << " bytes (" << chunks << " chunk(s)) never completed";

			std::ifstream is{ file, std::ios::binary };
			let actual = string{ std::istreambuf_iterator<char>{is}, std::istreambuf_iterator<char>{} };
			ASSERT_EQ( actual.size(), size );
			ASSERT_EQ( actual, content );
		}
	}

	// Regression for the Windows chunk-offset scheme: chunk 0 wrote at EOF (offset -1) while chunks
	// 1+ wrote at their absolute buffer offsets, so a multi-chunk append to a non-empty file
	// overwrote the head of the file and appended only the first chunk's bytes. The second write
	// must land wholly after the first.
	TEST_F( FileTests, AppendMultiChunk ){
		let chunkSize = IO::ChunkByteSize();
		let file = Tests::file( 400 );
		if( fs::exists(file) )
			fs::remove( file );
		let first = string( chunkSize*3+chunkSize/2, 'x' );
		let second = string( chunkSize*3+chunkSize/2, 'y' );
		for( const string* content : {&first, &second} ){ //pointers, not {first,second} - that copies both into the init-list.
			auto done = ms<std::atomic<bool>>();
			writeRaw( file, *content, done );
			ASSERT_TRUE( waitDone(*done) ) << "append of " << content->size() << " bytes never completed";
		}
		std::ifstream is{ file, std::ios::binary };
		let actual = string{ std::istreambuf_iterator<char>{is}, std::istreambuf_iterator<char>{} };
		ASSERT_EQ( actual, first+second );
	}

	// Regression: a zero-byte operation produced no chunks, so no completion ever arrived — the
	// awaiting coroutine hung and a completion poller was leaked. Empty writes must create the file
	// and resume; reads of empty files must resume with an empty string.
	TEST_F( FileTests, EmptyFile ){
		let file = Tests::file( 200 );
		if( fs::exists(file) )
			fs::remove( file );

		auto written = ms<std::atomic<bool>>();
		writeRaw( file, {}, written );
		ASSERT_TRUE( waitDone(*written) ) << "empty write never completed";
		ASSERT_TRUE( fs::exists(file) );
		ASSERT_EQ( fs::file_size(file), 0u );

		auto content = ms<string>( "sentinel" );
		auto readDone = ms<std::atomic<bool>>();
		readRaw( file, content, readDone );
		ASSERT_TRUE( waitDone(*readDone) ) << "empty read never completed";
		ASSERT_TRUE( content->empty() ) << "expected empty content, got: " << *content;
	}

	TEST_F( FileTests, CachedEmptyString ){
		let file = Tests::file( 300 );//never created on disk - a cache hit must not open the file.
		Cache::Set<string>( file.string(), "" );//explicit - `{}` is ambiguous between the T and sp<const T> overloads.
		auto content = ms<string>( "sentinel" );
		auto done = ms<std::atomic<bool>>();
		readRaw( file, content, done, true );
		ASSERT_TRUE( waitDone(*done) ) << "cached empty read never completed";
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
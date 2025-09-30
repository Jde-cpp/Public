#include <jde/fwk/io/FileAwait.h>
#include "../src/co/LockKey.h"
#include <boost/uuid.hpp>
#include <boost/endian/conversion.hpp>
#include <jde/fwk/process/execution.h>
#include <jde/fwk/utils/Vector.h>
#include <jde/fwk/process/thread.h>

#define let const auto

using boost::uuids::uuid;
namespace Jde::IO::Tests{
	constexpr ELogTags _tags{ ELogTags::Test };
	Ω File( uint index )->fs::path{
		let path = Settings::FindPath("/testing/file");
		return path
			? path->parent_path()/Ƒ( "{}{}{}", path->stem().string(), index, path->extension().string() )
			: fs::temp_directory_path()/Ƒ( "test{}.txt", index );
	}

	struct FileTests : public ::testing::Test{
	protected:
		FileTests() {}
		~FileTests() override{}

		Ω SetUpTestCase()ι->void{
			Execution::Run();//io strand threading issue.
		}
		α SetUp()->void override{
			INFO( "{}", File(0).string() );
			fs::create_directories( File(0).parent_path() );
		}
		α TearDown()->void override {}
	};

	Ω write( const fs::path& file, uuid guid1, uuid guid2, Vector<uuid>& written, bool createFile, SRCE )->LockKeyAwait::Task{
		auto l = co_await LockKeyAwait{ file.string() };
		[sl]( const fs::path& file, uuid guid1, uuid guid2, Vector<uuid>& written, [[maybe_unused]]CoLockGuard l, bool createFile )->VoidAwait::Task {
			try{
				co_await IO::WriteAwait{ file, Ƒ("{}\n{}\n", to_string(guid1), to_string(guid2)), createFile, Jde::ELogTags::Test, sl };
			}
			catch( IException& e ){
				e.Log();
				e.Throw();
			}
			written.push_back( guid1 );
			written.push_back( guid2 );
		}( file, guid1, guid2, written, move(l), createFile );
	}
	Ω read( const fs::path& file, Vector<uuid>& readValues, SRCE )->TAwait<string>::Task{
		let content = co_await IO::ReadAwait{ file, false, sl };
		let guidStrings = Str::Split( content, '\n' );
		ul l{ readValues.Mutex };
		for( auto&& guid : guidStrings ){
			try{
				readValues.push_back( boost::uuids::string_generator{}(string{guid}), l );
			}
			catch( const std::exception& e ){
				ERR( "[{}] Failed to parse GUID from string '{}': {}", file.string(), guid, e.what() );
			}
		}
	}

	Ω testFile( uint fileIndex )->void{
		array<uuid,1024> guids;
		for( uint i=0; i<guids.size(); ++i ){
			auto prefix = boost::endian::endian_reverse( fileIndex );
			((uint*)guids[i].data())[0] = prefix;
			auto suffix = boost::endian::endian_reverse( i );
			((uint*)guids[i].data())[1] = suffix;
		}

		Vector<uuid> written;
		let file = File(fileIndex);
		let exists = fs::exists(file);
		if( exists ){
			INFO( "Removing existing file: {}", file.string() );
			fs::remove( file );
		}
		SetThreadDscrptn( file.filename().string() );
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

	constexpr uint _fileSize{ 5 };
	TEST_F(FileTests, WriteRead){
		ASSERT_TRUE( IO::ChunkByteSize()<74 ); //guid+\n*2
		ASSERT_TRUE( IO::ThreadSize()>1 ); //guid+\n
		vector<std::jthread> threads;
		for( uint i=0; i<_fileSize; ++i )
			threads.emplace_back( [i](){testFile( i );} );
		for( auto& thread : threads )
				thread.join();
	}
}
#include <jde/fwk/log/MemoryLog.h>
#include <jde/fwk/chrono.h>
#include <jde/app/log/ProtoLog.h>
#define let const auto

namespace Jde::Opc::Gateway::Tests{
	struct LogTests : public ::testing::Test{
	protected:
		LogTests() {}
		~LogTests() override{}

		Ω SetUpTestCase()ι->void{ }
		α SetUp()->void override{}
		α TearDown()->void override {}
		α ProtoLog()ι->App::ProtoLog&{
			auto pp = find_if( Logging::Loggers(), []( auto& l ){ return dynamic_cast<App::ProtoLog*>(l.get())!=nullptr; } );
			return (App::ProtoLog&)**pp;
		}
	};

	TEST_F( LogTests, Exists ){
		auto entries = BlockAwait<TAwait<vector<App::Log::Proto::FileEntry>>,vector<App::Log::Proto::FileEntry>>( ProtoLog().Load() );
		ASSERT_TRUE( entries.size() );
	}
	TEST_F( LogTests, Archive ){
		Logging::Entry e{ SRCE_CUR, ELogLevel::Information, ELogTags::Test, "Test message" };
		let archiveFile = (*Settings::FindPath( "/logging/proto/path" ))/"2025/1/1/archive.binpb";
		if( fs::exists( archiveFile ) )
			fs::remove( archiveFile );
		e.Time = Chrono::ToTimePoint( 2025, 1, 1, 15 );
		ProtoLog().Write( e );
		for( int i=0; i<100; ++i ){
			ProtoLog().Write( {SRCE_CUR, ELogLevel::Information, ELogTags::Test, Ƒ("Test message - {}", i)} );
			if( fs::exists(archiveFile) )
				break;
		}
		ASSERT_TRUE( fs::exists(archiveFile) );
	}
}

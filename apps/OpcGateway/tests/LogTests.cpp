#include <jde/fwk/log/MemoryLog.h>
#include <jde/fwk/chrono.h>
#include <jde/fwk/io/FileAwait.h>
#include <jde/app/log/DailyLoadAwait.h>
#include <jde/app/log/LogQLAwait.h>
#include <jde/app/log/ProtoLog.h>
#include <jde/app/client/RemoteLog.h>
#include "../src/GatewayAppClient.h"
#define let const auto

namespace Jde::Opc::Gateway::Tests{
	constexpr ELogTags _tags{ ELogTags::Test };
	struct LogTests : public ::testing::Test{
	protected:
		LogTests() {}
		~LogTests() override{}

		Ω SetUpTestCase()ι->void{ }
		α SetUp()->void override{}
		α TearDown()->void override {}
		α ProtoLog()ι->App::ProtoLog&{ return *Logging::GetLogger<App::ProtoLog>(); }
	};

	TEST_F( LogTests, Exists ){
		auto entries = BlockAwait<TAwait<vector<App::Log::Proto::FileEntry>>,vector<App::Log::Proto::FileEntry>>( App::DailyLoadAwait(ProtoLog().DailyFile()) );
		ASSERT_TRUE( entries.size() );
	}
	TEST_F( LogTests, Archive ){
		Logging::Entry e{ SRCE_CUR, ELogLevel::Information, ELogTags::Test, "Test message" };
		let archiveFile = ( *Settings::FindPath("/logging/proto/path") )/"2025/1/1/archive.binpb";
		if( fs::exists(archiveFile) )
			fs::remove( archiveFile );
		e.Time = Chrono::ToTimePoint( 2025, 1, 1, 12 );
		ProtoLog().Write( e );
		for( int i=0; i<100; ++i ){
			ProtoLog().Write( {SRCE_CUR, ELogLevel::Information, ELogTags::Test, Ƒ("Test message - {}", i)} );
			if( fs::exists(archiveFile) )
				break;
		}
		std::this_thread::sleep_for( 1s );
		DBG( "archiveFile: {}", archiveFile.string() );
		ASSERT_TRUE( fs::exists(archiveFile) );
		auto content = BlockTAwait<string>( IO::ReadAwait(archiveFile) );
		ASSERT_NO_THROW( Protobuf::Deserialize<App::Log::Proto::ArchiveFile>(move(content)) );
	}

	TEST_F( LogTests, Remote ){
		Logging::Entry e{ SRCE_CUR, ELogLevel::Information, ELogTags::Test, "Test message" };
		App::Client::RemoteLog remote{ {{"delay", "PT0.001S"}}, AppClient() };
		remote.Write( e );
		remote.Shutdown( false );
		//std::this_thread::sleep_for( 1s );
	}

	TEST_F( LogTests, GraphQL ){
		let now = ToIsoString( Clock::now() );
		Logging::Entry eNow{ SRCE_CUR, ELogLevel::Information, ELogTags::Test, string{now} };
		Logging::Entry eHour{ SRCE_CUR, ELogLevel::Information, ELogTags::Test, ToIsoString(eNow.Time - 1h) };
		eHour.Time = eNow.Time - 1h;
		ProtoLog().Write( eHour );
		ProtoLog().Write( eNow );
		auto ql = "logs( time: {gt: $start} ){ text arguments level tags line time user{id} fileName functionName message id fileId functionId }";
		jobject vars{ {"start", ToIsoString(eHour.Time+1s)} };

		let entries = BlockTAwait<jarray>( App::LogQLAwait{move(QL::Parse(ql, vars, {}).Queries()[0])} );
		optional<jobject> jNow;
		for( let& log : entries ){
			let id = ToUuid( log.at("id").as_string() );
			if( id==eNow.Id() )
				jNow = log.as_object();
			ASSERT_FALSE( id==eHour.Id() ) << "Found hour log entry which should be excluded.";
		}
		ASSERT_TRUE( jNow );
	}
}
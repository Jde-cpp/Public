#include <jde/fwk/log/MemoryLog.h>
#include <jde/fwk/chrono.h>
#include <jde/fwk/io/FileAwait.h>
#include <jde/fwk/utils/Stopwatch.h>
#include <jde/web/client/socket/ClientSocketAwait.h>
#include <jde/app/log/DailyLoadAwait.h>
#include <jde/app/log/LogQLAwait.h>
#include <jde/app/log/ProtoLog.h>
#include <jde/app/client/RemoteLog.h>
#include "../src/GatewayAppClient.h"
#define let const auto

namespace Jde::Opc::Gateway::Tests{
	constexpr ELogTags _tags{ ELogTags::Test };
	struct Listener : public QL::IListener{
		Listener()ι:QL::IListener{ "LogTests" }{}
		α OnChange( const Jde::jvalue&, Jde::QL::SubscriptionId ) ε->void override{ ASSERT(false); }
		α OnTraces( App::Proto::FromServer::Traces&& traces )ι->void override{
			//ASSERT( traces.app_id() == AppClient()->AppId() );// would need to get it from db
			Received.insert( Received.end(), traces.values().begin(), traces.values().end() );
		}
		vector<App::Proto::FromServer::Trace> Received;
	};
	struct LogTests : public ::testing::Test{
	protected:
		LogTests() {}
		~LogTests() override{}

		Ω SetUpTestCase()ι->void{ }
		α SetUp()->void override{ _listener = sp<Listener>(new Listener{}); }
		α TearDown()->void override {}
		α ProtoLog()ι->App::ProtoLog&{ return Logging::GetLogger<App::ProtoLog>(); }

		sp<Listener> _listener;
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
		App::Client::RemoteLog remote{ {{"delay", "PT0.001S"}}, AppClient() };
		Logging::Entry e{ SRCE_CUR, ELogLevel::Information, ELogTags::Test, "Test message" };
		remote.Write( e );
		remote.Shutdown( false );
		std::this_thread::sleep_for( 1s );
	}

	TEST_F( LogTests, GraphQL ){
		let now = ToIsoString( Clock::now() );
		Logging::Entry eNow{ SRCE_CUR, ELogLevel::Information, ELogTags::Test, string{now} };
		Logging::Entry eHour{ SRCE_CUR, ELogLevel::Information, ELogTags::Test, ToIsoString(eNow.Time - 1h) };
		eHour.Time = eNow.Time - 1h;
		ProtoLog().Write( eHour );
		TRACE( "eHour.Time: {}", ToIsoString(eHour.Time) );
		ProtoLog().Write( eNow );
		const string start{ ToIsoString(eHour.Time+1s) };
		TRACE( "start: {}", start );
		auto ql = "logs( time: {gt: $start} ){ text arguments level tags line time user{id} fileName functionName message id fileId functionId }";
		jobject vars{ {"start", start} };

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
	TEST_F( LogTests, Subscribe ){
		auto ql = "subscription LogCreated{ logCreated(level: { gte: $level }, tags: $tags, start: $start){time text} }";
		jobject vars{
			{ "level", "Information" },
			{ "tags", jarray{"test"} },
			{ "start", ToIsoString(Clock::now() - 1min) }
		};
		auto subs = QL::ParseSubscriptions( move(ql), vars, {}, SRCE_CUR );
		auto l = subs.front().Fields.FindPtr<jobject>("level");
		ASSERT_TRUE( l );
		BlockAwait<Web::Client::ClientSocketAwait<jarray>, jarray>( AppClient()->Subscribe(move(ql), vars, _listener, SRCE_CUR) );
		App::Client::RemoteLog remote{ {{"delay", "PT0.001S"}}, AppClient() };
		let text = "Subscribe test message";
		Logging::Entry log{ SRCE_CUR, ELogLevel::Information, ELogTags::Test, text };
		remote.Write( log );
		Stopwatch sw;
		while( _listener->Received.empty() )
			ASSERT_NO_THROW( sw.CheckTimeout(600s, 1ms) );
		ASSERT_EQ( Protobuf::ToGuid(_listener->Received.back().message_id()), log.Id() );

		//TODO add logs before subscription to make sure they are retrieved.
		//Make sure only fields requested are returned.
		//Make sure meta data is correct.
	}
}
//The log tests that need a live gateway.  Everything that needed only Jde.App.Shared and a filesystem - the daily
//file, the archive rounds and the QL read-back - moved to libs/app/tests/LogTests.cpp.
#include <jde/fwk/log/MemoryLog.h>
#include <jde/fwk/chrono.h>
#include <jde/fwk/str.h>
#include <jde/fwk/io/protobuf.h>
#include <jde/fwk/utils/Stopwatch.h>
#include <jde/web/client/socket/ClientSocketAwait.h>
#include <jde/web/server/SubscribeLog.h>
#include <jde/app/client/RemoteLog.h>
#include "../src/GatewayAppClient.h" //!important
#include "utils/GatewayClientSocket.h"//!important
#define let const auto

namespace Jde::Opc::Gateway::Tests{
	constexpr ELogTags _tags{ ELogTags::Test };
	struct Listener final: public QL::IListener{
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

		sp<Listener> _listener;
	};

	TEST_F( LogTests, Remote ){
		App::Client::RemoteLog remote{ {{"delay", "PT0.001S"}}, AppClient() };
		Logging::Entry e{ SRCE_CUR, ELogLevel::Information, ELogTags::Test, "Test message" };
		remote.Write( e );
		remote.Shutdown();
		Process::RemoveShutdown( &remote );
		std::this_thread::sleep_for( 1s );
	}

	TEST_F( LogTests, Subscribe ){
		if( !Logging::FindLogger<Web::Server::SubscribeLog>() ){
			GTEST_SKIP() << "Need to fix logic of embedded appServer with subscription.";
		}
		auto ql = "subscription LogCreated{ logCreated(level: {gte: $level}, tags: $tags, start: $start){time text} }";
		jobject vars{
			{ "level", "Information" },
			{ "tags", jarray{"test"} },
			{ "start", ToIsoString(Clock::now() - 1min) }
		};
		auto subs = QL::ParseSubscriptions( move(ql), vars, {}, SRCE_CUR );
		auto l = subs.front().Fields.FindPtr<jobject>( "level" );
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
		Process::RemoveShutdown( &remote );
		//TODO add logs before subscription to make sure they are retrieved.
		//Make sure only fields requested are returned.
		//Make sure meta data is correct.
	}
	TEST_F( LogTests, LogTagsIntrospection ){
		auto q = "__type( name: \"logTags\" ){ enumValues{id name description} }";
		let value = Socket().QuerySync( move(q), {} );
		TRACE( "Received: {}", serialize(value) );
	}
}

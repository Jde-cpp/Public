#include <jde/fwk/chrono.h>
#include <jde/fwk/process/process.h>
#include "utils/helpers.h"
#include "../src/GatewayAppClient.h"

#define let const auto

namespace Jde::Opc::Gateway::Tests{
	constexpr ELogTags _tags{ ELogTags::Test };
	struct AppClientTests : ::testing::Test{

	};

	TEST_F( AppClientTests, ConnectionRow ){
		auto vars = jobject{
			{ "programName", "Tests.Opc" },
			{ "name", *Settings::FindString("/instanceName") }
		};
		auto q = "connections( programName: $programName, instanceName: $name ) { created }";
		auto await = AppClient()->Query<jarray>( move(q), move(vars) );
		let connections = BlockTAwait<jarray>( move(*await) );
		ASSERT_EQ( connections.size(), 1 );
		let startTime = string{ connections.at(0).as_object().at("created").get_string() };
		TRACE( "Process::StartTime: '{}', startTime: '{}'.", ToIsoString(Process::StartTime()), startTime );
		ASSERT_TRUE( std::chrono::abs(Process::StartTime()-Chrono::ToTimePoint(string{startTime})) < 600s );
	}

	TEST_F( AppClientTests, login ){
		using Web::FromServer::SessionInfo;
		auto payload = Process::GetEnv( "JDE_GOOGLE_JWT" );
		if( !payload )
			GTEST_SKIP() << "JDE_GOOGLE_JWT not set.";
		else{
			let value = BlockAwait<Web::Client::ClientSocketAwait<SessionInfo>,SessionInfo>( AppClient()->Login(Web::Jwt{*payload}, SL{}) );
			ASSERT_TRUE( value.session_id()>0 );
		}
	}
}

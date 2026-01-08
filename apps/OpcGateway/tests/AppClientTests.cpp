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
		ASSERT_TRUE( std::chrono::abs(Process::StartTime()-Chrono::ToTimePoint(string{startTime})) < 120s );
	}

	TEST_F( AppClientTests, Status ){
		auto q = "connections{id programName instanceName hostName created status{ memory values } }";
		auto await = AppClient()->Query<jarray>( move(q), {} );
		let connections = BlockTAwait<jarray>( move(*await) );
		TRACE( "Connections: {}", serialize(connections) );
		int count = 1;
		if( !Settings::FindBool("/testing/embeddedAppServer").value_or(true) )
			++count;
		if( !Settings::FindBool("/testing/embeddedOpcServer").value_or(true) )
			++count;
		ASSERT_EQ( connections.size(), count );
	}

	TEST_F( AppClientTests, Login ){
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

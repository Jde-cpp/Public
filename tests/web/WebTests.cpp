#include <execution>
#include <jde/web/flex/Flex.h>
#include "../../../Ssl/source/Ssl.h"
#include "../../../Framework/source/Stopwatch.h"

#define var const auto

namespace Jde::Web{
	static sp<Jde::LogTag> _logTag{ Logging::Tag( "tests" ) };

	struct WebTests : public ::testing::Test{
	protected:
		WebTests() {}
		~WebTests() override{}

		static α SetUpTestCase()->void;
		α SetUp()->void override{};
		α TearDown()->void override{}

		static string HeaderPayload;
		static string passcode;
		static string PublicKeyFile;
		static string PrivateKeyFile;
		static string CertificateFile;
	};
	string WebTests::HeaderPayload{ "secret stuff" };
	string WebTests::passcode{ "123456789" };
	string WebTests::PublicKeyFile{ _msvc ? (OSApp::ApplicationDataFolder() / "public.pem").string() : "/tmp/public.pem" };
	string WebTests::PrivateKeyFile{ _msvc ? (OSApp::ApplicationDataFolder() / "private.pem").string() : "/tmp/private.pem" };
	string WebTests::CertificateFile{ _msvc ? (OSApp::ApplicationDataFolder() / "cert.pem").string() : "/tmp/cert.pem" };

	
	α WebTests::SetUpTestCase()->void{
		Stopwatch _{ "WebTests::SetUpTestCase", _logTag };
		std::jthread thread( []{ 
			Flex::Start( 5005 ); 
		});
		thread.detach();
		while( !Flex::HasStarted() )
			std::this_thread::sleep_for( 100ms );
		DBG( "Flex started" );
	}

	TEST_F( WebTests, PingSsl ){
		var result = Ssl::Send<string>( "localhost", "/", "PING", "5005", "text/ping" );
		ASSERT_EQ( "SSL=true", result );
	}

	TEST_F( WebTests, PingPlain ){
		var result = Http::Send( "localhost", "/", "PING", "5005", {}, "text/ping" );
		ASSERT_EQ( "SSL=false", result );
	}
#pragma GCC diagnostic ignored "-Wuninitialized"
	TEST_F( WebTests, PingAttack ){
		constexpr uint count=1000;
		constexpr sv ContentType{ "application/x-www-form-urlencoded" };
		vector<uint> indexes( count );
		for( uint i=0; i<count; ++i )
			indexes[i] = i;
		SessionPK sessionIds[count]; 
		Stopwatch _{ "WebTests::PingAttack", _logTag };
		std::for_each( std::execution::par_unseq, indexes.begin(), indexes.end(), [sessionIds]( auto index )mutable{ 
			flat_map<string,string> headers{ {"authorization", ""} };
			var result = Http::Send( "localhost", Jde::format("/echo?{}", index), {}, "5005", {}, ContentType, http::verb::get, &headers );//TODO change to async
			var echoIndex = std::stoi( result );
			ASSERT_EQ( index, echoIndex );
			sessionIds[index] = std::stoi( headers["authorization"] );
		});
		//TODO Change to graph ql for session query.
		std::for_each( std::execution::par_unseq, indexes.begin(), indexes.end(), [sessionIds]( auto index )mutable{ 
			flat_map<string,string> headers{ {"authorization", ""} };
			var sessionId = sessionIds[index];
			Http::Send( "localhost", Jde::format("/echo?{}", sessionId), {}, "5005", std::to_string(sessionId), ContentType, http::verb::get, &headers );//TODO change to async
			ASSERT_EQ( sessionId, std::stoi(headers["authorization"]) );
		});
	}
	//close socket when making a request.
	//close the server in class.
	//test endpoint
	//test timeout reset and actual timeout
	//test exception in custom handler.
	//invalid sessionId.  --can't convert vs not exists.
//AppServer
		//test logout.

}
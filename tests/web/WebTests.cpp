#include <execution>
#include <jde/web/flex/Flex.h>
#include "../../../Ssl/source/Ssl.h"
#include "mocks/ServerMock.h"

#define var const auto

namespace Jde::Web{
	static sp<Jde::LogTag> _logTag{ Logging::Tag( "tests" ) };
	using Mock::Host; using Mock::Port;

	struct WebTests : ::testing::Test{
	protected:
		WebTests():_pRequestHandler(ms<Mock::RequestHandler>()) {}
		~WebTests() override{}

		Ω SetUpTestCase()->void;
		α SetUp()->void override{};
		α TearDown()->void override{}
		Ω TearDownTestCase()->void;

		sp<Flex::IRequestHandler> _pRequestHandler;
	};
	constexpr sv ContentType{ "application/x-www-form-urlencoded" };


	α WebTests::SetUpTestCase()->void{
		Stopwatch _{ "WebTests::SetUpTestCase", _logTag };
		Mock::Start();
	}

	α WebTests::TearDownTestCase()->void{
		Stopwatch _{ "WebTests::TearDownTestCase", _logTag };
		Mock::Stop();
	}

	TEST_F( WebTests, IsSsl ){
		var result = Ssl::Send<string>( Host, "/isSsl", {}, Port, "text/ping", {}, http::verb::get );
		ASSERT_EQ( "SSL=true", result );
	}

	TEST_F( WebTests, PingPlain ){
		flat_map<string,string> headers{ {"Summary", ""} };
		var result = Http::Send( Host, "/", "PING", Port, {}, "text/ping", http::verb::post, &headers );
		ASSERT_EQ( "SSL=false", headers["Summary"] );
	}

	TEST_F( WebTests, EchoAttack ){
		constexpr uint count=1000;
		vector<uint> indexes( count );
		for( uint i=0; i<count; ++i )
			indexes[i] = i;
		SessionPK sessionIds[count];
		Stopwatch _{ "WebTests::EchoAttack", _logTag };
		std::for_each( std::execution::par_unseq, indexes.begin(), indexes.end(), [&sessionIds]( auto index )mutable{
			flat_map<string,string> headers{ {"Authorization", ""} };
			var result = Http::Send( Host, Jde::format("/echo?{}", index), {}, Port, {}, ContentType, http::verb::get, &headers );//TODO change to async
			auto jsonResult = Json::Parse(result)["params"][0];
			var echoIndex = std::stoi( jsonResult.template get<string>() );
			ASSERT_EQ( index, echoIndex );
			sessionIds[index] = *Str::TryTo<uint>( headers["Authorization"], nullptr, 16 );
		});
		std::for_each( std::execution::par_unseq, indexes.begin(), indexes.end(), [&sessionIds]( auto index )mutable{
			flat_map<string,string> headers{ {"Authorization", ""} };
			var sessionId = sessionIds[index];
			Http::Send( Host, "/Authorization", {}, Port, Jde::format("{:x}", sessionId), ContentType, http::verb::get, &headers );
			ASSERT_EQ( sessionId, *Str::TryTo<uint>(headers["Authorization"], nullptr, 16) );
		});
	}
	TEST_F( WebTests, BadSessionId ){
		Stopwatch _{ "WebTests::BadSessionId", _logTag };
		try{
			var result = Http::Send( Host, "/echo?InvalidSessionId", {}, Port, "xxxx", ContentType, http::verb::get );
			ASSERT_FALSE( true );
		}
		catch( NetException& e){
			ASSERT_EQ( http::status::unauthorized, (http::status)e.Code );
		}
	}
	TEST_F( WebTests, CloseMidRequest ){
		Stopwatch _{ "WebTests::CloseMidRequest", _logTag };

		namespace beast = boost::beast;
		auto ioc = Flex::GetIOContext();
		tcp::resolver resolver{ *ioc };
    auto stream = mu<beast::tcp_stream>( *ioc );
		var results = resolver.resolve( Host, Port );
    stream->connect( results );
		uint delay = 1;
		http::request<http::empty_body> req{ http::verb::get, Jde::format("/delay?seconds={}", delay), 11 };
		req.set( http::field::content_type, ContentType );
		std::condition_variable_any cv;
		std::shared_mutex mtx;
		auto written = []( beast::error_code ec, uint bytes_transferred )ι{};
    http::async_write( *stream, req, written );
		auto read = [&]( beast::error_code ec, uint bytes_transferred )ι{
			if( ec )
				Flex::Fail( ec, "read" );
			sl l{ mtx };
			cv.notify_one();
		};
		beast::flat_buffer buffer;
		http::request_parser<http::string_body> parser;
		http::async_read( *stream, buffer, parser, read );
		boost::asio::post( *ioc, [&]{
			beast::error_code ec;
			stream->socket().shutdown( tcp::socket::shutdown_both, ec );
			ASSERT( !ec );
			stream->socket().close( ec );
			ASSERT( !ec );
			stream = nullptr;
			DBG( "~asio::post" );
		});
		sl l{ mtx };
		cv.wait( l );
		std::this_thread::sleep_for( std::chrono::seconds{delay}+500ms );
		//TODO find out why server is not getting the disconnect.
	}
	TEST_F( WebTests, BadTarget ){
		try{
			var result = Http::Send( Host, "/BadTarget", {}, Port );
		}
		catch( const NetException& e ){
			ASSERT_EQ( http::status::not_found, (http::status)e.Code );
		}
	}
	TEST_F( WebTests, BadAwaitable ){
		try{
			var result = Http::Send( Host, "/BadAwaitable", {}, Port );
			ASSERT_FALSE( true );
		}
		catch( const NetException& e ){
			ASSERT_EQ( http::status::internal_server_error, (http::status)e.Code );
		}
	}
	TEST_F( WebTests, TestTimeout ){
		Stopwatch sw{ "WebTests::TestTimeout", _logTag };
		var systemStartTime = Chrono::ToClock<Clock,steady_clock>( sw.StartTime() );
		var timeoutString = Settings::Get("http/timeout").value_or( "PT30S" );
		var timeout = Chrono::ToDuration( timeoutString );
		ASSERT( timeout<=30s );//too long to wait.
		flat_map<string,string> headers{ {"Authorization", ""} };
		auto j = Http::Send( Host, "/timeout", {}, Port, {}, ContentType, http::verb::get, &headers );
		var output = Json::Parse(j)["value"].template get<string>();
		var systemResult = Chrono::to_timepoint( output );
		//var steadyExpiration = Chrono::ToClock<steady_clock,Clock>( systemResult );
		DBG( "Expected:  '{}'  Actual:  '{}'", ToIsoString(systemStartTime+timeout), ToIsoString(systemResult) );
		ASSERT_LE( systemStartTime+timeout-1s, systemResult );

		var authorization = headers["Authorization"];
		j = Http::Send( Host, "/timeout", {}, Port, authorization );
		var nextSystemEndTime = Chrono::to_timepoint(Json::Parse(j)["value"].template get<string>());
		ASSERT_GT( nextSystemEndTime, systemStartTime );
		DBG( "newTimeout:  '{}'", ToIsoString(nextSystemEndTime) );

		std::this_thread::sleep_for( timeout+1s );
		DBG( "TestTimeout:  '{}'", ToIsoString(Clock::now()) );
		try{
			auto j = Http::Send( Host, "/timeout", {}, Port, authorization );
			ASSERT_FALSE( true );
		}
		catch( const NetException& e ){
			ASSERT_EQ( http::status::unauthorized, (http::status)e.Code );
		}
	}
	// TEST_F( WebTests, WebTest ){
	// 	Ssl::Send<string>( Host, "/timeout", {}, Port, ContentType, {}, http::verb::get );
	// 	BREAK;
	// 	std::this_thread::sleep_for( 20min );
	// }
	//test angular

//AppServer
		//test logout.
//Future:
	//keep alives
}
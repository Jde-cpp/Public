#include <execution>
#include <jde/web/client/http/ClientHttpAwait.h>
#include <jde/web/client/http/ClientHttpResException.h>
#include <jde/framework/chrono.h>
#include <jde/framework/Stopwatch.h>
#include <jde/framework/str.h>
#include <jde/framework/thread/execution.h>
#include "mocks/ServerMock.h"

#define let const auto

namespace Jde::Web{
	constexpr ELogTags _tags{ ELogTags::Test };
	using Mock::Host; using Mock::Port;

	struct WebTests : ::testing::Test{
	protected:
		WebTests():_requestHandler(ms<Mock::RequestHandler>(jobject{})) {}
		~WebTests() override{}

		Ω SetUpTestCase()->void;
		α SetUp()->void override{};
		α TearDown()->void override{}
		Ω TearDownTestCase()->void;

		sp<Server::IRequestHandler> _requestHandler;
	};
	constexpr sv ContentType{ "application/x-www-form-urlencoded" };
	up<IException> _pException;

	α WebTests::SetUpTestCase()->void{
		Stopwatch _{ "WebTests::SetUpTestCase", _tags };
		Mock::Start( Settings::AsObject("/http") );
	}

	α WebTests::TearDownTestCase()->void{
		Stopwatch _{ "WebTests::TearDownTestCase", _tags };
		Mock::Stop();
	}
	using Web::Client::ClientHttpAwait;
	using Web::Client::ClientHttpRes;
	using Web::Client::ClientHttpResException;

	TEST_F( WebTests, IsSsl ){
		auto await = ClientHttpAwait{ Host, "/ping", Port, {.ContentType="text/ping", .Verb=http::verb::post} };
		let res = BlockAwait<ClientHttpAwait,ClientHttpRes>( move(await) );
		//Debug( _tags, "Headers.Size: {}", res.Headers().size() );
		ASSERT_TRUE( res[http::field::server].contains("SSL") );
	}

	TEST_F( WebTests, GoogleCerts ){
		auto await = ClientHttpAwait{ "www.googleapis.com", "/oauth2/v3/certs", 443, {.ContentType="", .Verb=http::verb::get} };
		let res = BlockAwait<ClientHttpAwait,ClientHttpRes>( move(await) );
		let certs = res.Json();
		ASSERT_TRUE( certs.contains("keys") );
		let keys = certs.at("keys").as_array();
		ASSERT_GT( keys.size(), 0 );
		ASSERT_TRUE( keys[0].is_object() );
	}
	TEST_F( WebTests, GZip ){
		auto await = ClientHttpAwait{ "en.wikipedia.org", string{"/wiki/Madden_NFL_26"}, 443, {.ContentType="", .Verb=http::verb::get} };
		let res = BlockAwait<ClientHttpAwait,ClientHttpRes>( move(await) );
		ASSERT_TRUE( res[http::field::content_encoding].contains("gzip") );
	}

	TEST_F( WebTests, IsPlain ){
		auto await = ClientHttpAwait{ Host, "/ping", Port, {.ContentType="text/ping", .Verb=http::verb::post, .IsSsl=false} };
		let res = BlockAwait<ClientHttpAwait,ClientHttpRes>( move(await) );
		ASSERT_FALSE( res[http::field::server].contains("SSL") );
	}

	TEST_F( WebTests, EchoAttack ){
		constexpr uint count=200; //windows seems to limit Socket.Listen(backlog) to 200.
		array<uint,count> indexes;
		for( uint i=0; i<count; ++i )
			indexes[i] = i;
		array<SessionPK,count> sessionIds{};
		try{
			atomic<uint> connections = 0;
			std::for_each( indexes.begin(), indexes.end(), [&sessionIds,&connections]( uint index )mutable{
				[]( auto index, auto& sessionIds, auto& connections )->ClientHttpAwait::Task {
					if( _pException )
						co_return;
					auto pSessionIds = &sessionIds;
					const uint idx = index;
					try{
						++connections;
						ClientHttpRes res = co_await ClientHttpAwait{ Host, Ƒ("/echo?{}", idx), Port };
						--connections;
						auto jsonResult = Json::Parse( res.Body() )["params"].at(0);
						let echoIndex = To<SessionPK>( Json::AsString(jsonResult) );
						if( echoIndex!=idx )
							THROW( "index={} echoIndex={}", idx, echoIndex );
						(*pSessionIds)[idx] = *Str::TryTo<SessionPK>( res[http::field::authorization], nullptr, 16 );
					}
					catch( IException& e ){
						Debug( _tags, "connections={}", connections.load() );
						_pException = e.Move();
					}
				}( index, sessionIds, connections );
			});
			while( std::ranges::contains(sessionIds, 0) && !_pException )
				std::this_thread::yield();
			if( _pException )
				_pException->Throw();
			//std::for_each( std::execution::par_unseq, indexes.begin(), indexes.end(), [&sessionIds]( auto index )mutable{
			for_each( indexes, [&sessionIds]( auto index )mutable{
				[&sessionIds,index]()->ClientHttpAwait::Task{
					auto pSessionIds=&sessionIds;
					uint idx = index;
					let sessionId = (*pSessionIds)[idx];
					ClientHttpRes res = co_await ClientHttpAwait{ Host, "/Authorization", Port, {.Authorization=Ƒ("{:x}", sessionId)} };
					if( sessionId!=*Str::TryTo<uint>(res[http::field::authorization], nullptr, 16) )
						THROW( "sessionId={} authorization={}", sessionId, res[http::field::authorization] );
					(*pSessionIds)[idx] = 0;
				}();
			});
		}
		catch( const IException& e ){
			e.SetLevel( ELogLevel::Critical );
			e.Log();
			ASSERT_FALSE( true );
		}
		while( find_if(sessionIds, [](auto s){return s!=0;})!=sessionIds.end() )
			std::this_thread::yield();
	}
	TEST_F( WebTests, BadSessionId ){
		try{
			auto await = ClientHttpAwait{ Host, "/echo?InvalidSessionId", Port, {.Authorization="xxxxxx"} };
			let res = BlockAwait<ClientHttpAwait,ClientHttpRes>( move(await) );
			ASSERT_FALSE( true );
		}
		catch( ClientHttpResException& e){
			ASSERT_EQ( http::status::unauthorized, e.Status() );
		}
	}
	TEST_F( WebTests, CloseMidRequest ){
		namespace beast = boost::beast;
		net::any_io_executor strand = net::make_strand( *Executor() );
		tcp::resolver resolver{ strand };
    auto stream = mu<beast::tcp_stream>( strand );
		let results = resolver.resolve( Host, std::to_string(Port) );
    stream->connect( results );
		uint delay = 2;
		http::request<http::empty_body> req{ http::verb::get, Ƒ("/delay?seconds={}", delay), 11 };
		req.set( http::field::content_type, ContentType );
		std::condition_variable_any cv;
		std::shared_mutex mtx;
		auto onWrite = []( beast::error_code ec, uint /*bytes_transferred*/ )ι{
			ASSERT( !ec );
			Debug( _tags, "onWrite" );
		};
		net::post( strand, [&]{
    	http::async_write( *stream, req, onWrite );
		});
		auto onRead = [&]( beast::error_code ec, uint /*bytes_transferred*/ )ε{
			CodeException{ ec, _tags }; //expected.
			sl l{ mtx };
			cv.notify_one();
		};
		beast::flat_buffer buffer;
		http::request_parser<http::string_body> parser;
		http::async_read( *stream, buffer, parser, onRead );
		//std::this_thread::sleep_for( std::chrono::seconds{1} );
		net::post( strand, [&]{
			beast::error_code ec;
			stream->socket().shutdown( tcp::socket::shutdown_both, ec );//TODO use cancellation token.
			ASSERT( !ec );
			stream->socket().close( ec );
			ASSERT( !ec );
			stream = nullptr;
			Debug( _tags, "client stream shutdown" );
		});
		sl l{ mtx };
		cv.wait( l );
		std::this_thread::sleep_for( std::chrono::seconds{delay}+500ms );
		//TODO rest stream write succeeds even though stream is shutdown.
	}
	TEST_F( WebTests, BadTarget ){
		try{
			auto await = ClientHttpAwait{ Host, "/BadTarget", Port };
			let res = BlockAwait<ClientHttpAwait,ClientHttpRes>( move(await) );
		}
		catch( const ClientHttpResException& e ){
			ASSERT_EQ( http::status::not_found, e.Status() );
		}
	}
	TEST_F( WebTests, BadAwaitable ){
		try{
			auto await = ClientHttpAwait{ Host, "/BadAwaitable", Port };
			let res = BlockAwait<ClientHttpAwait,ClientHttpRes>( move(await) );
			ASSERT_FALSE( true );
		}
		catch( const ClientHttpResException& e ){
			ASSERT_EQ( http::status::internal_server_error, e.Status() );
		}
	}
	TEST_F( WebTests, TestTimeout ){
		let testStartTime = Chrono::ToClock<Clock,steady_clock>( steady_clock::now() );
		let timeoutString = Settings::FindSV("/http/timeout").value_or( "PT30S" );
		let timeout = Chrono::ToDuration( timeoutString );
		ASSERT( timeout<=30s );//too long to wait.

		let res = BlockAwait<ClientHttpAwait,ClientHttpRes>( ClientHttpAwait{Host, "/timeout", Port} );//fetch timeout
		let currentTimeoutString = Json::AsString( res.Json(), "value" );//
		let currentTimeout = Chrono::ToTimePoint( currentTimeoutString );
		DBG( "Expected: ({}+{}) '{}'  Actual:  '{}'", ToIsoString(testStartTime), timeoutString, ToIsoString(testStartTime+timeout), ToIsoString(currentTimeout) );
		ASSERT_LE( testStartTime+timeout-1s, currentTimeout );
		let authorization = res[http::field::authorization];

		auto await2 = ClientHttpAwait{ Host, "/timeout", Port, {.Authorization=authorization} };
		let res2 = BlockAwait<ClientHttpAwait,ClientHttpRes>( move(await2) );
		let nextSystemEndTime = Chrono::ToTimePoint( Json::AsString(Json::Parse(res2.Body()), "value") );
		ASSERT_GT( nextSystemEndTime, testStartTime );
		DBG( "newTimeout:  '{}'", ToIsoString(nextSystemEndTime) );

		std::this_thread::sleep_for( timeout+1s );
		DBG( "TestTimeout:  '{}'", ToIsoString(Clock::now()) );
		try{
			let res3 = BlockAwait<ClientHttpAwait,ClientHttpRes>( ClientHttpAwait{Host, "/timeout", Port, {.Authorization=authorization}} );
			ASSERT_FALSE( true );
		}
		catch( const ClientHttpResException& e ){
			ASSERT_EQ( http::status::unauthorized, e.Status() );
		}
	}
//TODO! gzip
//TODO Test redirect.
//TODO keep alives
//AppServer
		//TODO test logout.
}
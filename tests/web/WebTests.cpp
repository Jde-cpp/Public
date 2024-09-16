#include <execution>
#include <jde/web/client/http/ClientHttpAwait.h>
#include <jde/web/client/http/ClientHttpResException.h>
#include <jde/web/server/Flex.h>
#include "mocks/ServerMock.h"

#define var const auto

namespace Jde::Web{
	constexpr ELogTags _tags{ ELogTags::Test };
	static sp<Jde::LogTag> _logTag{ Logging::Tag( "test" ) };
	using Mock::Host; using Mock::Port;

	struct WebTests : ::testing::Test{
	protected:
		WebTests():_pRequestHandler(ms<Mock::RequestHandler>()) {}
		~WebTests() override{}

		Ω SetUpTestCase()->void;
		α SetUp()->void override{};
		α TearDown()->void override{}
		Ω TearDownTestCase()->void;

		sp<Server::IRequestHandler> _pRequestHandler;
	};
	constexpr sv ContentType{ "application/x-www-form-urlencoded" };
	up<IException> _pException;

	α WebTests::SetUpTestCase()->void{
		Stopwatch _{ "WebTests::SetUpTestCase", _tags };
		Mock::Start();
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
		var res = BlockAwait<ClientHttpAwait,ClientHttpRes>( move(await) );
		//Debug( _tags, "Headers.Size: {}", res.Headers().size() );
		ASSERT_TRUE( res[http::field::server].contains("SSL") );
	}

	TEST_F( WebTests, IsPlain ){
		auto await = ClientHttpAwait{ Host, "/ping", Port, {.ContentType="text/ping", .Verb=http::verb::post, .IsSsl=false} };
		var res = BlockAwait<ClientHttpAwait,ClientHttpRes>( move(await) );
		ASSERT_FALSE( res[http::field::server].contains("SSL") );
	}

	TEST_F( WebTests, EchoAttack ){
		constexpr uint count=200; //windows seems to limit Socket.Listen(backlog) to 200.
		array<uint,count> indexes;
		for( uint i=0; i<count; ++i )
			indexes[i] = i;
		array<SessionPK,count> sessionIds{};
		Stopwatch _{ "WebTests::EchoAttack", _tags };
		try{
			atomic<uint> connections = 0;
			std::for_each( std::execution::par_unseq, indexes.begin(), indexes.end(), [&sessionIds,&connections]( uint index )mutable{
				[index, &sessionIds,&connections]()->ClientHttpAwait::Task{
					if( _pException )
						co_return;
					auto pSessionIds = &sessionIds;
					const uint idx = index;
					try{
						++connections;
						ClientHttpRes res = co_await ClientHttpAwait{ Host, Ƒ("/echo?{}", idx), Port };
						--connections;
						auto jsonResult = Json::Parse( res.Body() )["params"][0];
						var echoIndex = To<SessionPK>( jsonResult.template get<string>() );
						if( echoIndex!=idx )
							THROW( "index={} echoIndex={}", idx, echoIndex );
						(*pSessionIds)[idx] = *Str::TryTo<SessionPK>( res[http::field::authorization], nullptr, 16 );
					}
					catch( IException& e ){
						Debug( _tags, "connections={}", connections.load() );
						_pException = e.Move();
					}
				}();
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
					var sessionId = (*pSessionIds)[idx];
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
		Stopwatch _{ "WebTests::BadSessionId", _tags };
		try{
			auto await = ClientHttpAwait{ Host, "/echo?InvalidSessionId", Port, {.Authorization="xxxxxx"} };
			var res = BlockAwait<ClientHttpAwait,ClientHttpRes>( move(await) );
			ASSERT_FALSE( true );
		}
		catch( ClientHttpResException& e){
			ASSERT_EQ( http::status::unauthorized, e.Status() );
		}
	}
	TEST_F( WebTests, CloseMidRequest ){
		Stopwatch _{ "WebTests::CloseMidRequest", _tags };

		namespace beast = boost::beast;
		net::any_io_executor strand = net::make_strand( *Executor() );
		tcp::resolver resolver{ strand };
    auto stream = mu<beast::tcp_stream>( strand );
		var results = resolver.resolve( Host, std::to_string(Port) );
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
			var res = BlockAwait<ClientHttpAwait,ClientHttpRes>( move(await) );
		}
		catch( const ClientHttpResException& e ){
			ASSERT_EQ( http::status::not_found, e.Status() );
		}
	}
	TEST_F( WebTests, BadAwaitable ){
		try{
			auto await = ClientHttpAwait{ Host, "/BadAwaitable", Port };
			var res = BlockAwait<ClientHttpAwait,ClientHttpRes>( move(await) );
			ASSERT_FALSE( true );
		}
		catch( const ClientHttpResException& e ){
			ASSERT_EQ( http::status::internal_server_error, e.Status() );
		}
	}
	TEST_F( WebTests, TestTimeout ){
		Stopwatch sw{ "WebTests::TestTimeout", _tags };
		var systemStartTime = Chrono::ToClock<Clock,steady_clock>( sw.StartTime() );
		var timeoutString = Settings::Get("http/timeout").value_or( "PT30S" );
		var timeout = Chrono::ToDuration( timeoutString );
		ASSERT( timeout<=30s );//too long to wait.

		auto await = ClientHttpAwait{ Host, "/timeout", Port };
		var res = BlockAwait<ClientHttpAwait,ClientHttpRes>( move(await) );
		var output = Json::Parse( res.Body() )["value"].template get<string>();
		var systemResult = Chrono::to_timepoint( output );
		DBG( "Expected:  '{}'  Actual:  '{}'", ToIsoString(systemStartTime+timeout), ToIsoString(systemResult) );
		ASSERT_LE( systemStartTime+timeout-1s, systemResult );
		var authorization = res[http::field::authorization];

		auto await2 = ClientHttpAwait{ Host, "/timeout", Port, {.Authorization=authorization} };
		var res2 = BlockAwait<ClientHttpAwait,ClientHttpRes>( move(await2) );
		var nextSystemEndTime = Chrono::to_timepoint(Json::Parse(res2.Body())["value"].template get<string>());
		ASSERT_GT( nextSystemEndTime, systemStartTime );
		DBG( "newTimeout:  '{}'", ToIsoString(nextSystemEndTime) );

		std::this_thread::sleep_for( timeout+1s );
		DBG( "TestTimeout:  '{}'", ToIsoString(Clock::now()) );
		try{
			var res3 = BlockAwait<ClientHttpAwait,ClientHttpRes>( ClientHttpAwait{Host, "/timeout", Port, {.Authorization=authorization}} );
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
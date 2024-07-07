#include "mocks/ServerMock.h"
#include "../../../Framework/source/math/MathUtilities.h"
#include "../../../Framework/source/io/AsioContextThread.h"
#include "../../../Ssl/source/Ssl.h"
#include "mocks/ClientSocketSessionMock.h"
#include <jde/web/flex/IHttpRequestAwait.h>
#include <jde/http/ClientSocketAwait.h>

#define var const auto
namespace Jde::Web{
	using Http::ClientSocketAwait;
	static sp<Jde::LogTag> _logTag{ Logging::Tag( "tests" ) };
	constexpr sv ContentType{ "application/x-www-form-urlencoded" };
	using Mock::Host; using Mock::Port;

	struct SocketTests : ::testing::Test{
	protected:
		SocketTests():_pRequestHandler(ms<Mock::RequestHandler>()) {}
		~SocketTests() override{}

		Ω SetUpTestCase()->void;
		α SetUp()->void override;
		α TearDown()->void override;
		Ω TearDownTestCase()->void;

		sp<Flex::IRequestHandler> _pRequestHandler;
	};

	sp<Mock::ClientSocketSession> _pSession{};
	SessionPK _sessionId;
	α SocketTests::SetUpTestCase()->void{
		Stopwatch _{ "SocketTests::SetUpTestCase", _logTag };
		Mock::Start();
	}
	α SocketTests::TearDownTestCase()->void{
		Stopwatch _{ "SocketTests::TearDownTestCase", _logTag };
		Mock::Stop();
	}

	α SocketTests::SetUp()->void{
		ClearMemoryLog();
	}
	#define NOTIFY sl l{ _mutex }; cv.notify_one()
	std::shared_mutex _mutex;
	std::condition_variable_any cv;
	α Notify()ι{
		sl l{ _mutex };
		cv.notify_one();
	}
	α Close()ι->VoidTask{
		co_await _pSession->Close();
		Notify();
	}
	#define WAIT sl l{ _mutex }; cv.wait( l )
	α Wait()ι{
		WAIT;
	}

	α SocketTests::TearDown()->void{
		if( _pSession ){
			Close();
			Wait();
			//ASSERT( _pSession.use_count()==1 );
			_pSession = nullptr;
		}
		_sessionId = 0;
	}

	up<IException> _exception;
	α Connect()->ClientSocketAwait<SessionPK>::Task{
		try{
			[[maybe_unused]] auto sessionId = co_await _pSession->Connect( _sessionId );
		}
		catch( IException& e ){
			_exception = e.Move();
		}
		NOTIFY;
	}
	α CreateSession( optional<ssl::context> ctx=nullopt )->VoidTask{
		if( _sessionId==0 ){
			flat_map<string,string> headers{ {"Authorization", ""} };
			Http::Send( Host, "/timeout", {}, Port, {}, ContentType, http::verb::get, &headers );
			_sessionId = *Str::TryTo<SessionPK>( headers["Authorization"], nullptr, 16 );
		}
		_pSession = ms<Mock::ClientSocketSession>( IO::AsioContextThread(), ctx );
		co_await _pSession->RunSession( Host, Port );
		Connect();
	}
	TEST_F( SocketTests, CreatePlain ){
		Stopwatch sw{ "WebTests::CreatePlain", _logTag };
		CreateSession();
		WAIT;
		ASSERT_EQ( _sessionId, _pSession->SessionId() );
	}

	TEST_F( SocketTests, CreateSsl ){
		Stopwatch sw{ "WebTests::CreateSsl", _logTag };
		CreateSession( ssl::context(ssl::context::tlsv12_client) );
		WAIT;
		ASSERT_EQ( _sessionId, _pSession->SessionId() );
	}

	flat_map<Http::RequestId,string> _requests; flat_map<Http::RequestId,string> _responses; mutex _echoMutex;
	α EchoText( uint requestId, string text )->ClientSocketAwait<string>::Task{
		string y = co_await _pSession->Echo( text );
		{
			lg _{ _echoMutex };
			_requests.emplace( requestId, move(text) );
			_responses.emplace( requestId, move(y) );
		}
	}
	TEST_F( SocketTests, EchoAttack ){
		Stopwatch sw{ "WebTests::EchoAttack", _logTag };
		CreateSession();
		WAIT;
		string text( 32000, 'a' );
		for( uint i=1; i<=1000; ++i )
			EchoText( i, text.substr(0,i*32) );
		for( ;; ){
			{
				lg _{ _echoMutex };
				if( _requests.size()==1000 )
					break;
			}
			std::this_thread::sleep_for( 10ms );
		}
		for( auto& [id, text] : _requests )
			ASSERT_EQ( text, _responses[id] );
	}

	TEST_F( SocketTests, BadSessionId ){
		_sessionId = Math::Random();
		CreateSession();
		WAIT;
		ASSERT_NE( nullptr, _exception );
	}

	TEST_F( SocketTests, CloseClientSide ){
		CreateSession();
		WAIT;
		Close();
		Wait();
		_pSession = nullptr;
		var msgId = Calc32RunTime( "[1]Server::DoRead - The WebSocket stream was gracefully closed at both endpoints" );
		std::this_thread::sleep_for( 100ms );
		auto logs = FindMemoryLog( msgId );
		ASSERT_TRUE( logs.size()>0 );
	}

	α CloseServerSideCall()ι->ClientSocketAwait<string>::Task{
		try{
		 [[maybe_unused]]	string y = co_await _pSession->CloseServerSide();
		}
		catch( IException& e ){
			_exception = e.Move();
		}
		NOTIFY;
	}

	TEST_F( SocketTests, CloseServerSide ){
		{ CreateSession(); WAIT; }
		CloseServerSideCall();
		WAIT;
		ASSERT_NE( nullptr, _exception );
		Close();
		Wait();
		ASSERT_EQ( _pSession.use_count(), 1 );//Read call & _pSession.
	}

	α BadTransmissionClientCall()ι->ClientSocketAwait<string>::Task{
		try{
			[[maybe_unused]] string y = co_await _pSession->BadTransmissionClient();
		}
		catch( IException& e ){
			_exception = e.Move();
		}
		NOTIFY;
	}
	TEST_F( SocketTests, BadTransmissionClient ){
		CreateSession();
		WAIT;
		BadTransmissionClientCall();
		var expiration = steady_clock::now() + 20s;
		vector<Logging::ExternalMessage> logs;
		while( logs.size()==0 && steady_clock::now()<expiration ){
			std::this_thread::sleep_for( 100ms );
			logs = FindMemoryLog( Calc32RunTime("Failed to process incomming exception '{}'.") );
		}
		ASSERT_TRUE( logs.size()>0 );
	}

	α BadTransmissionServerCall()ι->ClientSocketAwait<string>::Task{
		try{
			[[maybe_unused]] string y = co_await _pSession->BadTransmissionServer();
		}
		catch( IException& e ){
			_exception = e.Move();
		}
		NOTIFY;
	}
	TEST_F( SocketTests, BadTransmissionServer ){
		CreateSession();
		WAIT;
		BadTransmissionServerCall();
		var expiration = steady_clock::now() + 20s;
		vector<Logging::ExternalMessage> logs;
		while( logs.size()==0 && steady_clock::now()<expiration ){
			std::this_thread::sleep_for( 100ms );
			logs = FindMemoryLog( Calc32RunTime("MergePartialFromCodedStream returned false.") );//TODO send a exception to server.
		}
		ASSERT_TRUE( logs.size()>0 );
	}
}
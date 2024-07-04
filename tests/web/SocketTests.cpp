#include "../../../Framework/source/math/MathUtilities.h"
#include "../../../Ssl/source/Ssl.h"
#include "mocks/ServerMock.h"
#include "mocks/ClientSocketSessionMock.h"

#define var const auto
namespace Jde::Web{
	static sp<Jde::LogTag> _logTag{ Logging::Tag( "tests" ) };
	constexpr sv ContentType{ "application/x-www-form-urlencoded" };
	using Mock::Host; using Mock::Port;

	struct SocketTests : ::testing::Test{
	protected:
		SocketTests():_pRequestHandler(ms<Mock::RequestHandler>()) {}
		~SocketTests() override{}

		Ω SetUpTestCase()->void;
		α SetUp()->void override{};
		α TearDown()->void override{}
		Ω TearDownTestCase()->void;

		sp<Flex::IRequestHandler> _pRequestHandler;
	};

	sp<Mock::ClientSocketSession> _pSession;
	SessionPK _sessionId;
	α SocketTests::SetUpTestCase()->void{
		Stopwatch _{ "SocketTests::SetUpTestCase", _logTag };
		Mock::Start();
		_pSession = nullptr;
		_sessionId = 0;
		ClearMemoryLog();
	}

	α SocketTests::TearDownTestCase()->void{
		Stopwatch _{ "SocketTests::TearDownTestCase", _logTag };
		_pSession->Close();
		_pSession = nullptr;
		Mock::Stop();
	}
	std::shared_mutex _mutex;
	std::condition_variable_any cv;
	#define WAIT sl l{ _mutex }; cv.wait( l )
	#define NOTIFY sl l{ _mutex }; cv.notify_one()
	up<IException> _exception;
	α Connect()->Http::HttpTask<Http::Proto::FromServer::Ack>{
		try{
			Http::Proto::FromServer::Ack ack = co_await _pSession->Connect( _sessionId );
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
		_pSession = ms<Mock::ClientSocketSession>( Flex::GetIOContext(), ctx );
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
	α EchoText( string text )->Http::HttpTask<Http::Proto::Echo>{
		Http::Proto::Echo y = co_await _pSession->Echo( text );
		{
			lg _{ _echoMutex };
			_requests.emplace( y.request_id(), move(text) );
			_responses.emplace( y.request_id(), move(*y.mutable_echo_text()) );
		}
	}
	TEST_F( SocketTests, EchoAttack ){
		Stopwatch sw{ "WebTests::EchoAttack", _logTag };
		CreateSession();
		WAIT;
		string text( 32000, 'a' );
		for( uint i=1; i<=1000; ++i )
			EchoText( text.substr(0,i*32) );
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
		_pSession->Close();
		var msgId = Calc32RunTime( "[1]The WebSocket stream was gracefully closed at both endpoints" );
		std::this_thread::sleep_for( 100ms );
		auto logs = FindMemoryLog( msgId );
		ASSERT_TRUE( logs.size()>0 );
	}

	α CloseServerSideCall()ι->Http::HttpTask<Http::Proto::Echo>{
		try{
			Http::Proto::Echo y = co_await _pSession->CloseServerSide();
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
	}

	α BadTransmissionClientCall()ι->Http::HttpTask<Http::Proto::Echo>{
		try{
			Http::Proto::Echo y = co_await _pSession->BadTransmissionClient();
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
		vector<Logging::Messages::ServerMessage> logs;
		while( logs.size()==0 && steady_clock::now()<expiration ){
			std::this_thread::sleep_for( 100ms );
			logs = FindMemoryLog( Calc32RunTime("Failed to process incomming exception '{}'.") );
		}
		ASSERT_TRUE( logs.size()>0 );
	}

	α BadTransmissionServerCall()ι->Http::HttpTask<Http::Proto::Echo>{
		try{
			Http::Proto::Echo y = co_await _pSession->BadTransmissionServer();
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
		vector<Logging::Messages::ServerMessage> logs;
		while( logs.size()==0 && steady_clock::now()<expiration ){
			std::this_thread::sleep_for( 100ms );
			logs = FindMemoryLog( Calc32RunTime("MergePartialFromCodedStream returned false.") );//TODO send a exception to server.
		}
		ASSERT_TRUE( logs.size()>0 );
	}
}
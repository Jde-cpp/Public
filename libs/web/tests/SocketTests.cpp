//#include <boost/beast/ssl.hpp>
#include "mocks/ServerMock.h"
#include <jde/web/client/http/ClientHttpAwait.h>
#include <jde/web/client/Jwt.h>
#include "../../../../Framework/source/math/MathUtilities.h"
#include <jde/framework/thread/execution.h>
#include <jde/web/client/http/ClientHttpAwait.h>
#include "mocks/ClientSocketSession.h"
#include <jde/web/server/IHttpRequestAwait.h>
#include <jde/web/client/socket/ClientSocketAwait.h>

#define let const auto
namespace Jde::Web{
	constexpr ELogTags _tags{ ELogTags::Test };
	using Client::ClientSocketAwait;
	using Client::ClientHttpAwait;
	using Client::ClientHttpRes;

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

		sp<Server::IRequestHandler> _pRequestHandler;
	};

	sp<Mock::ClientSocketSession> _clientSession{};
	SessionPK _sessionId;
	α SocketTests::SetUpTestCase()->void{
		Stopwatch _{ "SocketTests::SetUpTestCase", ELogTags::Test };
		Mock::Start();
	}
	α SocketTests::TearDownTestCase()->void{
		Stopwatch _{ "SocketTests::TearDownTestCase", ELogTags::Test };
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
		co_await _clientSession->Close();
		Notify();
	}

	α Wait()ι{
		sl l{ _mutex }; 
		cv.wait( l );
	}

	α SocketTests::TearDown()->void{
		if( _clientSession ){
			Close();
			Wait();
			TRACE( "_clientSession.use_count()={}", _clientSession.use_count() );
			//ASSERT( _clientSession.use_count()==1 );
			_clientSession = nullptr;
			TRACE( "_clientSession.use_count()={}", _clientSession.use_count() );
		}
		_sessionId = 0;
	}

	up<IException> _exception;
	α Connect()->ClientSocketAwait<SessionPK>::Task{
		try{
			[[maybe_unused]] auto sessionId = co_await _clientSession->Connect( _sessionId );
		}
		catch( IException& e ){
			_exception = e.Move();
		}
		NOTIFY;
	}
	α CreateSession( optional<ssl::context> ctx=nullopt )->VoidTask{
		if( _sessionId==0 ){
			Crypto::CryptoSettings settings{ "http/ssl" };
			auto [mod,exp] = Crypto::ModulusExponent( settings.PublicKeyPath );
			Web::Jwt jwt{ move(mod), move(exp), "testUser", "testUserCallSign", "127.0.0.1", {}/*description*/, settings.PrivateKeyPath };
			auto await = ClientHttpAwait{ Host, "/loginCertificate", serialize(jobject{{"jwt", jwt.Payload()}}), Port };
			let res = BlockAwait<ClientHttpAwait,ClientHttpRes>( move(await) );
			_sessionId = *Str::TryTo<SessionPK>( res[http::field::authorization], nullptr, 16 );
			Information( ELogTags::Test, "({:x})Loggin Complete.", _sessionId );//TODOBuild change to test
		}
		_clientSession = ms<Mock::ClientSocketSession>( Executor(), ctx );
		co_await _clientSession->RunSession( Host, Port );
		Connect();
	}
	TEST_F( SocketTests, CreatePlain ){
		Stopwatch sw{ "WebTests::CreatePlain", ELogTags::Test };
		CreateSession();
		Wait();
		ASSERT_EQ( _sessionId, _clientSession->SessionId() );
	}

	TEST_F( SocketTests, CreateSsl ){
		std::this_thread::sleep_for( 1s );
		Trace( ELogTags::Test, "WebTests::CreateSsl" );
		Stopwatch sw{ "WebTests::CreateSsl", ELogTags::Test };
		CreateSession( ssl::context(ssl::context::tlsv12_client) );
		Wait();
		ASSERT_EQ( _sessionId, _clientSession->SessionId() );
	}

	flat_map<RequestId,string> _requests; flat_map<RequestId,string> _responses; mutex _echoMutex;
	α EchoText( uint requestId, string text )->ClientSocketAwait<string>::Task{
		string y = co_await _clientSession->Echo( text );
		{
			lg _{ _echoMutex };
			_requests.emplace( requestId, move(text) );
			_responses.emplace( requestId, move(y) );
		}
	}
	TEST_F( SocketTests, EchoAttack ){
		Stopwatch sw{ "WebTests::EchoAttack", ELogTags::Test };
		CreateSession();
		Wait();
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
		Wait();
		ASSERT_NE( nullptr, _exception );
	}

	TEST_F( SocketTests, CloseClientSide ){
		CreateSession();
		Wait();
		Close();
		Wait();
		let sessionId = _clientSession->Id();
		_clientSession = nullptr;
		std::this_thread::sleep_for( 1s );
		auto logs = FindMemoryLog( [=](const Logging::ExternalMessage& m){
			return m.Args.size()==3 && m.Args[0]=="1" && m.Args[1]=="The WebSocket stream was gracefully closed at both endpoints" && m.Args[2]==Ƒ("[{:x}]Server::DoRead", sessionId);
		});
		ASSERT_TRUE( logs.size()>0 );
		std::this_thread::sleep_for( 100ms );
	}

	α CloseServerSideCall()ι->ClientSocketAwait<string>::Task{
		try{
		 [[maybe_unused]]	string y = co_await _clientSession->CloseServerSide();
		}
		catch( IException& e ){
			_exception = e.Move();
		}
		NOTIFY;
	}

	TEST_F( SocketTests, CloseServerSide ){
		{ CreateSession(); Wait(); }
		CloseServerSideCall();
		Wait();
		ASSERT_NE( nullptr, _exception );
	}

	α BadTransmissionClientCall()ι->ClientSocketAwait<string>::Task{
		try{
			[[maybe_unused]] string y = co_await _clientSession->BadTransmissionClient();
		}
		catch( IException& e ){
			_exception = e.Move();
		}
		NOTIFY;
	}
	TEST_F( SocketTests, BadTransmissionClient ){
		CreateSession();
		Wait();
		BadTransmissionClientCall();
		let expiration = steady_clock::now() + 20s;
		vector<Logging::ExternalMessage> logs;
		while( logs.size()==0 && steady_clock::now()<expiration ){
			std::this_thread::sleep_for( 100ms );
			logs = FindMemoryLog( Calc32RunTime("Failed to process incomming exception '{}'.") );
		}
		Trace{ ELogTags::Test, "logs.size(): {}", logs.size() };
		ASSERT_TRUE( logs.size()>0 );
	}

	α BadTransmissionServerCall()ι->ClientSocketAwait<string>::Task{
		try{
			[[maybe_unused]] string y = co_await _clientSession->BadTransmissionServer();
		}
		catch( IException& e ){
			_exception = e.Move();
		}
		NOTIFY;
	}
	TEST_F( SocketTests, BadTransmissionServer ){
		CreateSession();
		Wait();
		BadTransmissionServerCall();
		let expiration = steady_clock::now() + 20s;
		vector<Logging::ExternalMessage> logs;
		while( logs.size()==0 && steady_clock::now()<expiration ){
			std::this_thread::sleep_for( 100ms );
			logs = FindMemoryLog( Calc32RunTime("MergePartialFromCodedStream returned false.") );//TODO send a exception to server.
		}
		ASSERT_TRUE( logs.size()>0 );
	}
}
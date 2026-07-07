//#include <boost/beast/ssl.hpp>
#include "jde/fwk/co/Await.h"
#include "jde/fwk/usings.h"
#include "mocks/ServerMock.h"
#include <jde/web/client/http/ClientHttpAwait.h>
#include <jde/web/Jwt.h>
#include <jde/fwk/utils/mathUtils.h>
#include <jde/fwk/utils/Stopwatch.h>
#include <jde/fwk/log/MemoryLog.h>
#include <jde/fwk/process/execution.h>
#include "mocks/ClientSocketSession.h"
#include <jde/web/server/IHttpRequestAwait.h>
#include <jde/web/client/socket/ClientSocketAwait.h>

#define let const auto
namespace Jde::Web{
	constexpr ELogTags _tags{ ELogTags::Test };
	using Client::ClientSocketAwait;
	using Client::ClientHttpAwait;
	using Client::ClientHttpRes;

	using Mock::Host; using Mock::Port;

	struct SocketTests : ::testing::Test{
	protected:
		SocketTests():_pRequestHandler(ms<Mock::RequestHandler>(jobject{})) {}
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
		Mock::Start( Settings::AsObject("/http") );
	}
	α SocketTests::TearDownTestCase()->void{
		Stopwatch _{ "SocketTests::TearDownTestCase", ELogTags::Test };
		Mock::Stop();
	}

	α SocketTests::SetUp()->void{
		Logging::ClearMemory();
	}
	#define NOTIFY sl l{ _mutex }; cv.notify_one()
	std::shared_mutex _mutex;
	std::condition_variable_any cv;
	α Notify()ι{
		sl l{ _mutex };
		cv.notify_one();
	}
	α Close()ι->VoidTask{
		co_await _clientSession->Close( true, SRCE_CUR );
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

	up<Exception> _exception;
	Ω connect()->SessionPK{
		return BlockAwait<ClientSocketAwait<SessionPK>,SessionPK>( _clientSession->Connect( _sessionId ) );
	}
	Ω createSession( optional<ssl::context> ctx=nullopt )->void{
		if( _sessionId==0 ){
			Crypto::CryptoSettings settings{ "http/ssl" };
			auto publicKey = Crypto::ReadPublicKey( settings.PublicKeyPath );
			Web::Jwt jwt{ move(publicKey), {0}, "testUser", "testUserCallSign", 0, "127.0.0.1", Clock::now()+1h, {}/*description*/, settings.PrivateKeyPath };
			auto await = ClientHttpAwait{ Host, "/login", serialize(jobject{{"jwt", jwt.Payload()}}), Port };
			let res = BlockAwait<ClientHttpAwait,ClientHttpRes>( move(await) );
			_sessionId = *Str::TryTo<SessionPK>( res[http::field::authorization], nullptr, 16 );
			INFO( "({:x})Loggin Complete.", _sessionId );
		}
		_clientSession = ms<Mock::ClientSocketSession>( Executor(), ctx );
		BlockVoidAwait( _clientSession->RunSession( Host, Port ) );
		connect();
	}
	TEST_F( SocketTests, CreatePlain ){
		Stopwatch sw{ "WebTests::CreatePlain", ELogTags::Test };
		createSession();
		ASSERT_EQ( _sessionId, _clientSession->SessionId() );
	}

	TEST_F( SocketTests, CreateSsl ){
		std::this_thread::sleep_for( 1s );
		TRACET( ELogTags::Test, "WebTests::CreateSsl" );
		Stopwatch sw{ "WebTests::CreateSsl", ELogTags::Test };
		createSession( ssl::context(ssl::context::tlsv12_client) );
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
		createSession();
		constexpr uint payloadBase = 32;
		constexpr uint size = 1000;
		string text( payloadBase*size, 'a' );
		std::this_thread::sleep_for( 1ms );
		TRACE( "----------------------------------------------------------------" );
		for( uint i=1; i<=size; ++i ){
			EchoText( i, text.substr(0,i*payloadBase) );
		}
		for( ;; ){
			{
				lg _{ _echoMutex };
				if( _requests.size()==size )
					break;
			}
			std::this_thread::sleep_for( 10ms );
		}
		for( auto&& [id, text] : _requests )
			ASSERT_EQ( text, _responses[id] );
	}

	TEST_F( SocketTests, BadSessionId ){
		_sessionId = Math::Random();
		EXPECT_THROW(createSession(), Exception);
	}

	TEST_F( SocketTests, CloseClientSide ){
		createSession();
		Close();
		Wait();
		let sessionId = _clientSession->Id();
		_clientSession = nullptr;
		std::this_thread::sleep_for( 1s );
		auto logs = Logging::Find( [=](const Logging::Entry& m){
			return m.Text.contains("The WebSocket stream was gracefully closed at both endpoints")
				&& m.Arguments.size()==1 && m.Arguments[0]==Ƒ( "[{:x}]Server::DoRead", sessionId );
		});
		ASSERT_TRUE( logs.size()>0 );
		std::this_thread::sleep_for( 100ms );
	}

	TEST_F( SocketTests, CloseServerSide ){
		createSession();
		EXPECT_THROW( (BlockAwait<ClientSocketAwait<string>,string>( _clientSession->CloseServerSide() )), Exception );
	}

	α BadTransmissionClientCall()ι->ClientSocketAwait<string>::Task{
		try{
			//never returns because server can't read it, TODO add a timeout.
			co_await _clientSession->BadTransmissionClient();
		}
		catch( Exception& e ){
			_exception = e.Move();
		}
		NOTIFY;
	}

	TEST_F( SocketTests, BadTransmissionClient ){
		createSession();
		BadTransmissionClientCall();
		 
		let expiration = steady_clock::now() + 60s;
		vector<Logging::Entry> logs;
		let id = Crypto::CalcMd5( "[{}]Failed to process incoming exception '{}'."sv );
		while( logs.size()==0 && steady_clock::now()<expiration ){
			std::this_thread::sleep_for( 100ms );
			logs = Logging::Find( id );
		}
		TRACET( ELogTags::Test, "logs.size(): {}", logs.size() );
		ASSERT_TRUE( logs.size()>0 );
	}

	α BadTransmissionServerCall()ι->ClientSocketAwait<string>::Task{
		try{
			[[maybe_unused]] string y = co_await _clientSession->BadTransmissionServer();
		}
		catch( Exception& e ){
			_exception = e.Move();
		}
		NOTIFY;
	}
	TEST_F( SocketTests, BadTransmissionServer ){
		createSession();
		BadTransmissionServerCall();
		let expiration = steady_clock::now() + 20s;
		vector<Logging::Entry> logs;
		while( logs.size()==0 && steady_clock::now()<expiration ){
			std::this_thread::sleep_for( 100ms );
			logs = Logging::Find( Crypto::CalcMd5("MergePartialFromCodedStream returned false."sv) );//TODO send a exception to server.
		}
		ASSERT_TRUE( logs.size()>0 );
	}
}
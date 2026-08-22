//#include <boost/beast/ssl.hpp>
#include <jde/web/client/ClientSsl.h>
#include "jde/fwk/co/Await.h"
#include "jde/fwk/usings.h"
#include "mocks/ServerMock.h"
#include <jde/web/client/http/ClientHttpAwait.h>
#include <jde/web/Jwt.h>
#include <jde/fwk/chrono.h>
#include <jde/fwk/utils/mathUtils.h>
#include <jde/fwk/utils/Stopwatch.h>
#include <jde/fwk/log/MemoryLog.h>
#include <jde/fwk/process/execution.h>
#include "mocks/ClientSocketSession.h"
#include <jde/web/server/IHttpRequestAwait.h>
#include <jde/web/server/Sessions.h>
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
	//_notified + a predicate, not a bare notify_one: a completion that lands before Wait() blocks is otherwise lost and the test
	//hangs instead of the code.  That happens whenever a close or a request finishes synchronously - which C3 and C6 both made
	//routine.
	#define NOTIFY sl l{ _mutex }; _notified = true; cv.notify_one()
	std::shared_mutex _mutex;
	std::condition_variable_any cv;
	bool _notified{};
	α Notify()ι{
		sl l{ _mutex };
		_notified = true;
		cv.notify_one();
	}
	α Close()ι->VoidTask{
		co_await _clientSession->Close( true, SRCE_CUR );
		Notify();
	}

	α Wait()ι{
		sl l{ _mutex };
		cv.wait( l, []{ return _notified; } );
		_notified = false;
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
	Ω login()->void{
		if( _sessionId )
			return;
		Crypto::CryptoSettings settings{ "http/ssl" };//the client signs the jwt with its own key pair - the server's 'web.tests' identity is not the caller's.
		if( !fs::exists(settings.PrivateKey.Path) || !fs::exists(settings.PublicKey.Path) ){//first run on a machine; no certificate needed, the mock '/login' verifies the signature against the jwt's key.
			settings.CreateDirectories();
			Crypto::CreateKey( settings, SRCE_CUR );
		}
		auto publicKey = Crypto::ReadPublicKey( settings.PublicKey.Path );
		Web::Jwt jwt{ move(publicKey), {0}, "testUser", "testUserCallSign", 0, "127.0.0.1", Clock::now()+1h, {}/*description*/, settings.PrivateKey };
		auto await = ClientHttpAwait{ Host, "/login", serialize(jobject{{"jwt", jwt.Payload()}}), Port };
		let res = BlockAwait<ClientHttpAwait,ClientHttpRes>( move(await) );
		_sessionId = *Str::TryTo<SessionPK>( res[http::field::authorization], nullptr, 16 );
		INFO( "({:x})Loggin Complete.", _sessionId );
	}
	Ω connectSocket( optional<ssl::context> ctx=nullopt )->void{
		_clientSession = ms<Mock::ClientSocketSession>( Executor(), ctx );
		BlockVoidAwait( _clientSession->RunSession( Host, Port ) );
		connect();
	}
	Ω createSession( optional<ssl::context> ctx=nullopt )->void{
		login();
		connectSocket( move(ctx) );
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
		createSession( Client::Ssl::MakeContext() );
		ASSERT_EQ( _sessionId, _clientSession->SessionId() );
	}

	//A raw session id is only a credential from the endpoint it was minted for.  Resuming it from a different address must be
	//denied - otherwise a guessed or leaked id is a full account takeover (appserver-review2 #3, Sessions.cpp UpdateExpiration).
	//Driven through the Sessions API rather than the socket because both ends of a localhost socket share 127.0.0.1.
	TEST_F( SocketTests, SessionEndpointBinding ){
		using Server::SessionInfo; using Server::Sessions::UpsertAwait;
		let info = Server::Sessions::Add( Jde::UserPK{7}, "10.9.9.1", true );//session bound to 10.9.9.1.
		let id = Ƒ("{:x}", info->SessionId);
		let resume = []( str authorization, str endpoint )->sp<SessionInfo> {
			return BlockAwait<UpsertAwait,sp<SessionInfo>>( UpsertAwait{ authorization, endpoint, true, nullptr } );
		};
		EXPECT_THROW( resume(id, "10.9.9.2"), Exception ) << "a session id must not resume from a different endpoint";
		let same = resume( id, "10.9.9.1" );
		ASSERT_TRUE( same ) << "the minting endpoint must still resume the session";
		EXPECT_EQ( info->SessionId, same->SessionId );
		Server::Sessions::Remove( info->SessionId );
	}

	flat_map<RequestId,string> _requests; flat_map<RequestId,string> _responses; flat_map<RequestId,string> _echoFailures; mutex _echoMutex;
	//Records the outcome either way.  A failed echo used to skip the emplace, so EchoAttack's wait on _requests.size() could never
	//be met and the suite hung instead of failing - which ql-review3 #16's message cap made routine.
	α EchoText( uint requestId, string text )->ClientSocketAwait<string>::Task{
		string response; optional<string> failure;
		try{
			response = co_await _clientSession->Echo( text );
		}
		catch( std::exception& e ){
			failure = e.what();
		}
		lg _{ _echoMutex };
		_requests.emplace( requestId, move(text) );
		if( failure )
			_echoFailures.emplace( requestId, move(*failure) );
		else
			_responses.emplace( requestId, move(response) );
	}
	//A session created over rest carries the rest timeout; the socket connecting on it must promote it to /http/socketTimeout.
	TEST_F( SocketTests, SocketPromotesSessionTimeout ){
		Stopwatch sw{ "SocketTests::SocketPromotesSessionTimeout", ELogTags::Test };
		let restTimeout = Chrono::ToDuration( Settings::FindSV("/http/timeout").value_or("PT30S") );
		let socketTimeout = Chrono::ToDuration( Settings::FindSV("/http/socketTimeout").value_or("P1D") );
		ASSERT_GT( socketTimeout, restTimeout*2 );//need an unambiguous gap to tell the two apart.

		login();
		let authorization = Ƒ( "{:x}", _sessionId );
		let expiration = [&authorization]()->TimePoint{//the mock '/timeout' target echos SessionInfo::Expiration.
			let res = BlockAwait<ClientHttpAwait,ClientHttpRes>( ClientHttpAwait{Host, "/timeout", Port, {.Authorization=authorization}} );
			return Chrono::ToTimePoint( Json::AsString(res.Json(), "value") );
		};

		let restStart = Chrono::ToClock<Clock,steady_clock>( steady_clock::now() );
		let restExpiration = expiration();
		DBG( "rest expiration: '{}', expected before '{}'", ToIsoString(restExpiration), ToIsoString(restStart+restTimeout+1s) );
		ASSERT_LT( restExpiration, restStart+restTimeout+1s );//no socket yet.

		connectSocket();
		let socketStart = Chrono::ToClock<Clock,steady_clock>( steady_clock::now() );
		let socketExpiration = expiration();//a rest request, so this also proves the promotion is sticky.
		DBG( "socket expiration: '{}', expected after '{}'", ToIsoString(socketExpiration), ToIsoString(socketStart+restTimeout+1s) );
		ASSERT_GT( socketExpiration, socketStart+restTimeout+1s );
		ASSERT_LE( socketExpiration, socketStart+socketTimeout+1s );
	}

	TEST_F( SocketTests, EchoAttack ){
		Stopwatch sw{ "WebTests::EchoAttack", ELogTags::Test };
		createSession();
		//ql-review3 #16 caps a socket message at /http/bodyLimit (Streams.cpp read_message_max), so the largest echo has to stay
		//under it: past the cap the server drops the session and every request in flight fails - OversizeMessageClosesSocket
		//covers that.  The envelope is the protobuf framing around echo_text: a few tags and lengths plus the request id.
		let bodyLimit = Settings::FindNumber<uint>( "/http/bodyLimit" ).value_or( 0 );
		constexpr uint size = 1000;
		constexpr uint envelope = 64;
		ASSERT_GT( bodyLimit, envelope+size ) << "Web.Tests.jsonnet sets /http/bodyLimit; the echoes are sized from it";
		let payloadBase = (bodyLimit-envelope)/size;
		string text( payloadBase*size, 'a' );
		{
			lg _{ _echoMutex };
			_requests.clear(); _responses.clear(); _echoFailures.clear();
		}
		std::this_thread::sleep_for( 1ms );
		TRACE( "----------------------------------------------------------------" );
		for( uint i=1; i<=size; ++i ){
			EchoText( i, text.substr(0,i*payloadBase) );
		}
		//Bounded, so a stranded echo fails the test instead of hanging it.  Each request already has /web/client/socketRequestTimeout
		//to be answered before the client fails it, so this only trips if that watchdog did not release one.
		let deadline = steady_clock::now()+30s;
		for( ;; ){
			{
				lg _{ _echoMutex };
				if( _requests.size()==size )
					break;
				ASSERT_LT( steady_clock::now(), deadline ) << Ƒ( "{} of {} echoes completed", _requests.size(), size );
			}
			std::this_thread::sleep_for( 10ms );
		}
		ASSERT_TRUE( _echoFailures.empty() ) << Ƒ( "{} echoes failed, first [{}]: {}", _echoFailures.size(), _echoFailures.begin()->first, _echoFailures.begin()->second );
		for( auto&& [id, text] : _requests )
			ASSERT_EQ( text, _responses[id] );
	}

	//ql-review3 #16: a socket message is capped at /http/bodyLimit like an http body.  Past it Beast fails the server's read with
	//message_too_big and the session is dropped, so the client sees its request fail and OnClose run - not a 413, and not a hang:
	//EchoAttack used to send 32KB echoes and wait forever for answers that could never come.
	TEST_F( SocketTests, OversizeMessageClosesSocket ){
		let limit = Settings::FindNumber<uint>( "/http/bodyLimit" ).value_or( 0 );
		ASSERT_TRUE( limit ) << "Web.Tests.jsonnet sets /http/bodyLimit; without it there is no cap to exceed";
		createSession();
		let sessionId = _clientSession->Id();
		//extra parens: the comma in BlockAwait<A,B> would otherwise split the macro's argument list.
		EXPECT_THROW( (BlockAwait<ClientSocketAwait<string>,string>( _clientSession->Echo(string(limit+1024, 'a')) )), Exception );
		for( uint i=0; i<100 && !_clientSession->OnCloseCount(); ++i )
			std::this_thread::sleep_for( 10ms );
		EXPECT_EQ( _clientSession->OnCloseCount(), 1u );
		//and it was the cap that ended it, not the request deadline.
		vector<Logging::Entry> logs;
		for( uint i=0; i<100 && logs.empty(); ++i ){
			logs = Logging::Find( [=](const Logging::Entry& m){
				return m.Text.contains("exceeded the locally configured limit") && m.Arguments.size()==1 && m.Arguments[0]==Ƒ( "[{:x}]Server::DoRead", sessionId );
			});
			if( logs.empty() )
				std::this_thread::sleep_for( 10ms );
		}
		EXPECT_FALSE( logs.empty() ) << "the server should have refused the message as too big";
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

	//C3: OnClose nulls the stream on the strand while Write/Close run on other threads.  Everything reaches it through StreamPtr()
	//now, and a second close finds null - which has to resume the await rather than dereference it or hang the caller.  TearDown
	//closes again after this, so the test also covers a third.
	TEST_F( SocketTests, CloseAfterClose ){
		createSession();
		Close();
		Wait();
		//BlockVoidAwait rather than the Close()/Wait() pair above: the second close completes synchronously now (await_ready sees a
		//closed/closing stream), and Wait()'s condition_variable has no predicate - Notify() would fire before it blocks and the
		//lost wakeup would hang the test rather than the code.  Before this, Suspend went straight at a nulled _stream.
		BlockVoidAwait( _clientSession->Close(true, SRCE_CUR) );
		_clientSession = nullptr;//TearDown's Close()/Wait() would hit the same lost wakeup.
	}

	//app-review2 #2: the server closing first must run the *whole* client teardown, not just drain _tasks.  Beast auto-replies
	//to the received close frame, so no async_close of ours ever completes and nothing else will ever call OnClose - which is
	//where _stream/_ioContext are nulled and where an app session drops its dead session and reconnects.  Draining _tasks alone
	//left the gateway wedged after every routine AppServer restart: still Connected(), never reconnecting.
	TEST_F( SocketTests, CloseServerSide ){
		createSession();
		EXPECT_THROW( (BlockAwait<ClientSocketAwait<string>,string>( _clientSession->CloseServerSide() )), Exception );
		for( uint i=0; i<100 && !_clientSession->OnCloseCount(); ++i )
			std::this_thread::sleep_for( 10ms );
		EXPECT_EQ( _clientSession->OnCloseCount(), 1u );//exactly once - firing it here as well as from an in-flight async_close would run a derived session's reconnect twice.
	}

	α BadTransmissionClientCall()ι->ClientSocketAwait<string>::Task{
		try{
			//C6: the server answers this with an exception carrying a requestId the client cannot match, so nothing ever resumes
			//the caller.  It used to hang here forever; /web/client/socketRequestTimeout now ends it.
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
		//C6: and the stranded caller is now released rather than left waiting - the request deadline fails it.
		Wait();
		ASSERT_TRUE( _exception!=nullptr ) << "the unanswerable request never completed";
		_exception = nullptr;
		_clientSession = nullptr;//the request deadline already closed this session.
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
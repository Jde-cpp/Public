#include "mocks/ServerMock.h"
#include "mocks/ClientSocketSessionMock.h"

namespace Jde::Web{
	static sp<Jde::LogTag> _logTag{ Logging::Tag( "tests" ) };
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

	α SocketTests::SetUpTestCase()->void{
		Stopwatch _{ "SocketTests::SetUpTestCase", _logTag };
		Mock::Start();
	}

	α SocketTests::TearDownTestCase()->void{
		Stopwatch _{ "SocketTests::TearDownTestCase", _logTag };
		Mock::Stop();
	}

	std::shared_mutex _mutex;
	sp<ClientSocketSession> _pSession;
	α Connect(){
		flat_map<string,string> headers{ {"Authorization", ""} };
		Http::Send( Host, "/timeout", {}, Port, {}, ContentType, http::verb::get, &headers );
		var authorization = *Str::TryTo<SessionPK>( headers["Authorization"], nullptr, 16 );
		pSession = ms<ClientSocketSession>( Flex::GetIOContext() );
		pSession->Run( Host, Port );
		SessionPK sessionId = co_await p->Connect( authorization );
		sl l{ _mutex };
		cv.notify_one();
	}
	TEST_F( SocketTests, CreatePlain ){
		Stopwatch sw{ "WebTests::BadSessionId", _logTag };

		p->Write( FromClient::Create(authorization) );
		sl l{ mtx };
		cv.wait( l );
		ASSERT_NE( 0, _pSession->SessionId() );
	}
//BadSessionId
//request_id

	TEST_F( SocketTests, CreateSsl ){
		//connect socket.
		//Add common proto
		//request_id
	}

	TEST_F( SocketTests, Echo ){
		//request_id
	}

	TEST_F( SocketTests, EchoAttack ){
		//request_id
	}
	TEST_F( SocketTests, BadSessionId ){

	}
	TEST_F( SocketTests, BigData ){
	}

	TEST_F( SocketTests, CloseClientSide ){
	}

	TEST_F( SocketTests, CloseServerSide ){
	}

	TEST_F( SocketTests, BadTransmission ){
	}

}
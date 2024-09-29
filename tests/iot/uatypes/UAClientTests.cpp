#include <jde/iot/UM.h>
#include <jde/io/Json.h>
#include <jde/iot/types/OpcServer.h>
#include <jde/iot/uatypes/UAClient.h>
#include <jde/crypto/OpenSsl.h>
#include <open62541/plugin/create_certificate.h>
#include "../helpers.h"

#include "../../../../Framework/source/um/UM.h"

#define var const auto

namespace Jde::Iot{
	constexpr ELogTags _tags{ ELogTags::Test };

	struct UAClientTests : public ::testing::Test{
	protected:
		UAClientTests()ι{}
		~UAClientTests()override{}

		Ω SetUpTestCase()ι->void;
		α SetUp()ι->void override{}
		α TearDown()ι->void override{}
		Ω TearDownTestSuite();
	public:
		static json OpcServer;
	};
	json UAClientTests::OpcServer{};

	α UAClientTests::SetUpTestCase()ι->void{
		OpcServer = Iot::SelectOpcServer();
		if( OpcServer.empty() ){
			atomic_flag done;
			[](atomic_flag& done)->Task {
				co_await ProviderAwait{ OpcServerTarget, false };//remove dangling.
				[](atomic_flag& done)->CreateOpcServerAwait::Task {
					auto id = co_await CreateOpcServerAwait();
					UAClientTests::OpcServer = Iot::SelectOpcServer( id );
					done.test_and_set();
					done.notify_one();
				}(done);
			}( done );
			done.wait( false );
		}
		Trace( _tags, "OpcServer={}", OpcServer.dump() );
	}

	α UAClientTests::TearDownTestSuite(){
		Iot::PurgeOpcServer();
	}

	std::condition_variable_any cv;
	std::shared_mutex mtx;
	sp<UAClient> _pClient;
	vector<SessionPK> _sessionIds;
	up<IException> _exception;
	const string _password = "0123456789ABCD";
	α AuthenticateTest( bool badPassword=false )ι->Iot::AuthenticateAwait::Task{
		try{
			OpcNK opcId = Json::Getε( UAClientTests::OpcServer, "target" );
			auto sessionInfo = co_await Iot::AuthenticateAwait{ "user1", badPassword ? "xyz" : _password, UAClientTests::OpcServer["target"], "localhost", true };
			_sessionIds.push_back( sessionInfo.session_id() );
		}
		catch( IException& e ){
			_exception = e.Move();
		}
		std::shared_lock l{ mtx };
		cv.notify_one();
	}

	TEST_F( UAClientTests, Authenticate ){
		Information( _tags, "UAClientTests.Authenticate" );
		AuthenticateTest();
		{
			std::shared_lock l{ mtx };
			cv.wait( l );
		}
		AuthenticateTest();
		AuthenticateTest();
		std::shared_lock l{ mtx };
		cv.wait( l );
		cv.wait( l );
		var creds = Credentials( _sessionIds[2], OpcServer["target"] );
		EXPECT_EQ( "user1", get<0>(creds) );
		EXPECT_EQ( _password, get<1>(creds) );
		EXPECT_NE( _sessionIds[0], _sessionIds[1] );
		EXPECT_NE( _sessionIds[0], _sessionIds[2] );
		EXPECT_NE( _sessionIds[1], _sessionIds[2] );
		EXPECT_TRUE( find(_sessionIds, 0)==_sessionIds.end() );
		Information( _tags, "~UAClientTests.Authenticate" );
	}

	TEST_F( UAClientTests, Authenticate_BadPassword ){
		Information( _tags, "UAClientTests.Authenticate_BadPassword" );
		AuthenticateTest( true );
		std::shared_lock l{ mtx };
		cv.wait( l );
		//EXPECT_FALSE( _pClient );
		EXPECT_TRUE( _exception );
		EXPECT_TRUE( _exception && string{_exception->what()}.contains("BadUserAccessDenied") );
		Debug( _tags, "{}", _exception ? _exception->what() : "Error no exception." );
		Information( _tags, "~UAClientTests.Authenticate_BadPassword" );
	}
}
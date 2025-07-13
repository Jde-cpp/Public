#include <jde/framework/io/json.h>
#include <jde/crypto/OpenSsl.h>
#include "Auth.h"
#include "../../src/auth/TokenAwait.h"
#include "../../src/auth/OpcServerSession.h"

#define let const auto

namespace Jde::Opc::Gateway::Tests{
	constexpr ELogTags _tags{ ELogTags::Test };

	class TokenTests : public Auth{
	protected:
		TokenTests()ι:Auth{UA_USERTOKENTYPE_ISSUEDTOKEN}{}
		~TokenTests()override{}
		α TearDown()ι->void override{}
		Ω TearDownTestSuite();
	};

	α TokenTests::TearDownTestSuite(){
		PurgeOpcClient();
	}
	std::condition_variable_any cv;
	std::shared_mutex mtx;
	//sp<UAClient> _client;
	//vector<SessionPK> _sessionIds;
	up<IException> _exception;
	Ω authenticateTest( OpcClientNK opcId, bool bad=false )ι->TokenAwait::Task{
		try{
			co_await TokenAwait{ bad ? "xyz" : "abc", move(opcId), "localhost", true };
			//_sessionIds.push_back( sessionInfo.session_id() );
		}
		catch( IException& e ){
			_exception = e.Move();
		}
		std::shared_lock l{ mtx };
		cv.notify_one();
	}

	TEST_F( TokenTests, Authenticate ){
		string opcId{ Json::AsString(OpcServer,"target") };
		authenticateTest( opcId );
		{
			std::shared_lock l{ mtx };
			cv.wait( l );
		}
		authenticateTest( opcId );
		authenticateTest( opcId );
		std::shared_lock l{ mtx };
		cv.wait( l );
		cv.wait( l );
//		THROW_IF( _sessionIds.size()!=3, "Expected 3 sessions, found {}.", _sessionIds.size() );
//		let creds = GetCredential( _sessionIds[2], opcId );
//		EXPECT_EQ( "user1", get<0>(creds) );
//		EXPECT_EQ( _password, get<1>(creds) );
//		EXPECT_NE( _sessionIds[0], _sessionIds[1] );
//		EXPECT_NE( _sessionIds[0], _sessionIds[2] );
//		EXPECT_NE( _sessionIds[1], _sessionIds[2] );
//		EXPECT_TRUE( find(_sessionIds, 0)==_sessionIds.end() );
	}

	TEST_F( TokenTests, Authenticate_BadPassword ){
		authenticateTest( Json::AsString(OpcServer,"target"), true );
		std::shared_lock l{ mtx };
		cv.wait( l );
		//EXPECT_FALSE( _client );
		EXPECT_TRUE( _exception );
		EXPECT_TRUE( _exception && string{_exception->what()}.contains("BadUserAccessDenied") );
		Debug( _tags, "{}", _exception ? _exception->what() : "Error no exception." );
	}
}
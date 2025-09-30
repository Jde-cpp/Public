#include <jde/fwk/crypto/OpenSsl.h>
#include "../../src/auth/PasswordAwait.h"
#include "Auth.h"


#define let const auto

namespace Jde::Opc::Gateway::Tests{
	constexpr ELogTags _tags{ ELogTags::Test };

	class PasswordTests : public Auth{
	protected:
		PasswordTests()ι:Auth{ETokenType::Username}{}
		~PasswordTests()override{}

		//Ω SetUpTestCase()ε->void;
		//Ω CheckPasswordsAllowed()ε->void;
		//α SetUp()ι->void override;
		α TearDown()ι->void override{}
		Ω TearDownTestSuite();
	};

	α PasswordTests::TearDownTestSuite(){
		Auth::TearDownTestSuite();
	}
	static std::condition_variable_any cv;
	static std::shared_mutex mtx;
	static vector<SessionPK> _sessionIds;
	static up<IException> _exception;
	const string _password = "0123456789ABCD";
	α AuthenticateTest( ServerCnnctnNK opcId, bool badPassword=false )ι->TAwait<optional<Web::FromServer::SessionInfo>>::Task{
		try{
			auto sessionInfo = co_await PasswordAwait{ "user1", badPassword ? "xyz" : _password, move(opcId), "localhost", 0, true };
			if( sessionInfo )
				_sessionIds.push_back( sessionInfo->session_id() );
		}
		catch( IException& e ){
			_exception = e.Move();
		}
		std::shared_lock l{ mtx };
		cv.notify_one();
	}

	TEST_F( PasswordTests, Authenticate ){
		INFO( "PasswordTests.Authenticate" );
		string opcId{ Connection->Target };
		AuthenticateTest( opcId );
		{
			std::shared_lock l{ mtx };
			cv.wait( l );
		}
		AuthenticateTest( opcId );
		AuthenticateTest( opcId );
		std::shared_lock l{ mtx };
		cv.wait( l );
		cv.wait( l );
		THROW_IF( _sessionIds.size()!=3, "Expected 3 sessions, found {}.", _sessionIds.size() );
		let creds = GetCredential( _sessionIds[2], opcId );
		ASSERT_TRUE( creds );
		EXPECT_EQ( "user1", creds->LoginName() );
		EXPECT_EQ( _password, creds->Password() );
		EXPECT_NE( _sessionIds[0], _sessionIds[1] );
		EXPECT_NE( _sessionIds[0], _sessionIds[2] );
		EXPECT_NE( _sessionIds[1], _sessionIds[2] );
		EXPECT_TRUE( find(_sessionIds, 0)==_sessionIds.end() );
		INFO( "~PasswordTests.Authenticate" );
	}

	TEST_F( PasswordTests, Authenticate_BadPassword ){
		INFO( "PasswordTests.Authenticate_BadPassword" );
		AuthenticateTest( Connection->Target, true );
		std::shared_lock l{ mtx };
		cv.wait( l );
		EXPECT_TRUE( _exception );
		EXPECT_TRUE( _exception && string{_exception->what()}.contains("BadUserAccessDenied") );
		DBG( "{}", _exception ? _exception->what() : "Error no exception." );
		INFO( "~PasswordTests.Authenticate_BadPassword" );
	}
}
#include <jde/framework/io/json.h>
#include <jde/crypto/OpenSsl.h>
#include "Auth.h"
#include "../../src/auth/TokenAwait.h"
#include "../../src/auth/OpcServerSession.h"
#include "../../src/GatewayAppClient.h"

#define let const auto

namespace Jde::Opc::Gateway::Tests{
	constexpr ELogTags _tags{ ELogTags::Test };

	class TokenTests : public Auth{
	protected:
		TokenTests()ι:Auth{ETokenType::IssuedToken}{}
		~TokenTests()override{}
		Ω SetUpTestCase()ε->void;
		α TearDown()ι->void override{}
		Ω TearDownTestSuite();
		static optional<Web::Jwt> _jwt;
	};
	optional<Web::Jwt> TokenTests::_jwt;

	α TokenTests::SetUpTestCase()ε->void{
		_jwt = BlockAwait<Web::Client::ClientSocketAwait<Jde::Web::Jwt>,Web::Jwt>( AppClient()->Jwt() );
	}

	α TokenTests::TearDownTestSuite(){
		Auth::TearDownTestSuite();
	}

	up<IException> _exception;
	Ω authenticateTest( const Web::Jwt& jwt, ServerCnnctnNK opcId, atomic_flag& flag, bool bad=false )ι->TAwait<sp<UAClient>>::Task{
		try{
			auto text = jwt.Payload();
			if( bad )
				text[42] = text[42]-1;
			auto client = co_await UAClient::GetClient( move(opcId), Credential{text} );
			//co_await TokenAwait{ text, move(opcId), "localhost", true };
			//_sessionIds.push_back( sessionInfo.session_id() );
		}
		catch( IException& e ){
			_exception = e.Move();
		}
		flag.test_and_set();
		flag.notify_all();
		TRACE( "notify_all" );
	}

	TEST_F( TokenTests, Authenticate ){
		let opcId{ Connection->Target };
		atomic_flag a,b,c;
		authenticateTest( *_jwt, opcId, a );
		a.wait( false );
		TRACE( "Call b" );
		authenticateTest( *_jwt, opcId, b );
		TRACE( "Call c" );
		authenticateTest( *_jwt, opcId, c );
		b.wait( false );
		TRACE( "b returned" );
		c.wait( false );
		TRACE( "c returned" );
		EXPECT_FALSE( _exception );
		std::this_thread::sleep_for( 100ms );
	}

	TEST_F( TokenTests, Authenticate_Bad ){
		atomic_flag flag;
		authenticateTest( *_jwt, Connection->Target, flag, true );
		flag.wait( false );
		EXPECT_TRUE( _exception );
		EXPECT_TRUE( _exception && string{_exception->what()}.contains("BadIdentityTokenInvalid") );
		Debug( _tags, "{}", _exception ? _exception->what() : "Error no exception." );
	}
}
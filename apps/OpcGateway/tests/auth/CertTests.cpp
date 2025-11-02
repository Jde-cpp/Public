#include <jde/fwk/io/json.h>
#include <jde/fwk/crypto/OpenSsl.h>
#include "Auth.h"
#include "../../src/auth/CertAwait.h"
#include "../../src/auth/OpcServerSession.h"
#include "../../src/GatewayAppClient.h"

#define let const auto

namespace Jde::Opc::Gateway::Tests{
	constexpr ELogTags _tags{ ELogTags::Test };

	class CertTests : public Auth{
	protected:
		CertTests()ι:Auth{ETokenType::Certificate}{}
		~CertTests()override{}
		Ω SetUpTestCase()ε->void;
		α TearDown()ι->void override{}
		Ω TearDownTestSuite();

		α Connect( atomic_flag& flag, char id )ι->ConnectAwait::Task;
		optional<Credential> _cred;
		up<IException> _exception;
		sp<UAClient> _client;
	};

	α CertTests::SetUpTestCase()ε->void{

	}
	α CertTests::TearDownTestSuite(){
		Auth::TearDownTestSuite();
	}

	α CertTests::Connect( atomic_flag& flag, char id )ι->ConnectAwait::Task{
		try{
			TRACE( "Call {}", id );
			_client = co_await UAClient::GetClient( Connection->Target, Credential{Crypto::PublicKey{}} );
			ASSERT( _client );
			//co_await CertAwait{ Client->Target, "localhost", true };
			TRACE( "{} returned", id );
		}
		catch( IException& e ){
			TRACE( "{} failed", id );
			_exception = e.Move();
		}
		flag.test_and_set();
		flag.notify_all();
	}

	TEST_F( CertTests, Authenticate ){
		string opcId{ Connection->Target };
		atomic_flag a,b,c,d;
		Connect( a, 'a' );//test Connection.
		Connect( b, 'b' );//test waiting for a.
		a.wait( false );
		b.wait( false );
		Connect( c, 'c' );//test already have connection.
		Connect( d, 'd' );
		c.wait( false );
		d.wait( false );
		EXPECT_FALSE( _exception );
		UAClient::RemoveClient( move(_client) );
	}

	TEST_F( CertTests, Authenticate_Bad ){
		let root = Process::AppDataFolder();
		let working = root/"ssl";
		if( fs::exists(working) && !fs::exists(root/"ssl_backup") )
			fs::rename( working, root/"ssl_backup" );
		if( fs::exists(root/"ssl_badTest") )
			fs::rename( root/"ssl_badTest", working );
		else{
			Crypto::CryptoSettings settings{ jobject{} };
			settings.CreateDirectories();
			Crypto::CreateKeyCertificate( settings );
		}
		atomic_flag flag;
		Connect( flag, 'a' );
		flag.wait( false );

		EXPECT_TRUE( _exception );
		EXPECT_TRUE( _exception && string{_exception->what()}.contains("BadIdentityTokenInvalid") );
		EXPECT_FALSE( _client );
		DBG( "{}", _exception ? _exception->what() : "Error no exception." );
		fs::rename( working, root/"ssl_badTest" );
		fs::rename( root/"ssl_backup", working );
	}
}
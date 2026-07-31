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
		α TearDown()ι->void override{
			if( _client ){
				UAClient::RemoveClient( move(_client) );
				_client = nullptr;
			}
		}
		Ω TearDownTestSuite();

		α Connect( atomic_flag& flag, char id )ι->ConnectAwait::Task;
		optional<Credential> _cred;
		up<Exception> _exception;
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
		catch( Exception& e ){
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
	}

	TEST_F( CertTests, Authenticate_Bad ){
		let root = Process::AppDataFolder();
		let working = root/"ssl";
		if( fs::exists(working) && !fs::exists(root/"ssl_backup") )
			fs::rename( working, root/"ssl_backup" );
		if( fs::exists(root/"ssl_badTest") )
			fs::rename( root/"ssl_badTest", working );
		struct Restore final{ //the real certs have to come back even when the body throws - otherwise every later run starts on the bad ssl dir.
			fs::path Root; fs::path Working;
			~Restore(){
				std::error_code ec;
				fs::rename( Working, Root/"ssl_badTest", ec );
				fs::rename( Root/"ssl_backup", Working, ec );
			}
		} restore{ root, working };
		//the untrusted key has to land on the paths the client presents from - a default-named cert leaves the configured one missing, and the read happens in the noexcept UAClient::Create, so that terminates the process.
		Crypto::EnsureKeyCertificate( *AppClient()->SslSettings );

		atomic_flag flag;
		Connect( flag, 'a' );
		flag.wait( false );

		EXPECT_TRUE( _exception );
		EXPECT_TRUE( _exception && string{_exception->what()}.contains("BadSecurityChecksFailed") );
		EXPECT_FALSE( _client );
		DBG( "{}", _exception ? _exception->what() : "Error no exception." );
	}
}
#include <jde/fwk/io/json.h>
#include <jde/fwk/crypto/OpenSsl.h>
#include "Auth.h"
#include "../../src/auth/CertAwait.h"
#include "../../src/auth/OpcServerSession.h"
#include "../../src/GatewayAppClient.h"

#define let const auto

namespace Jde::Opc::Gateway::Tests{
	constexpr ELogTags _tags{ ELogTags::Test };

	//no connection needed - EnsureCertificate is a static that only touches the cert tree.
	struct CertFileTests : ::testing::Test{
		static constexpr sv Target{ "certUriChangeTest" };
		α TearDown()ι->void override{
			std::error_code ec;
			fs::remove( UAClient::CryptoSettings(ServerCnnctnNK{Target}).Certificate.Path, ec );
		}
	};
	//the cert file name keys on the target, its SAN on the certificateUri - editing a connection's uri must re-issue,
	//or the gateway presents a stale SAN forever and every session is refused BadCertificateUriInvalid.
	TEST_F( CertFileTests, ReissuesWhenCertificateUriChanges ){
		let path = UAClient::CryptoSettings( ServerCnnctnNK{Target} ).Certificate.Path;
		let sanUri = [&]{ return Crypto::Certificate{ Crypto::ReadCertificate(path) }.SanUri(); };

		UAClient::EnsureCertificate( ServerCnnctnNK{Target}, "urn:first.application" );
		ASSERT_TRUE( fs::exists(path) );
		EXPECT_EQ( sanUri(), "urn:first.application" );

		UAClient::EnsureCertificate( ServerCnnctnNK{Target}, "urn:second.application" );//same file name, different uri.
		EXPECT_EQ( sanUri(), "urn:second.application" );

		//and it must NOT churn when nothing changed - re-issuing every connect would rotate a cert peers have trusted.
		let before = Crypto::ReadCertificate( path );
		UAClient::EnsureCertificate( ServerCnnctnNK{Target}, "urn:second.application" );
		EXPECT_EQ( Crypto::ReadCertificate(path), before );
	}

	//server-side counterpart to ReissuesWhenCertificateUriChanges: the OpcServer must trust a transport cert re-issued
	//AFTER its startup snapshot (UATrust rescans on a failed verify) - pre-fix every secured connect fails
	//BadCertificateUntrusted until the server restarts. IssuedToken auth, not Certificate: certAuth swaps the transport
	//cert to AppClient()->SslSettings (Configuration()), while every other credential presents the per-target issued
	//file - the production scenario. (Not Username - the test server doesn't offer that policy.)
	class TrustReloadTests : public Auth{
	protected:
		TrustReloadTests()ι:Auth{ETokenType::IssuedToken}{}
		Ω SetUpTestCase()ε->void{ _jwt = BlockAwait<Web::Client::ClientSocketAwait<Jde::Web::Jwt>,Web::Jwt>( AppClient()->Jwt() ); }
		α TearDown()ι->void override{
			if( _client )
				UAClient::RemoveClient( move(_client) );
		}
		α Connect( atomic_flag& flag )ι->ConnectAwait::Task;
		static optional<Web::Jwt> _jwt;
		up<Exception> _exception;
		sp<UAClient> _client;
	};
	optional<Web::Jwt> TrustReloadTests::_jwt;

	α TrustReloadTests::Connect( atomic_flag& flag )ι->ConnectAwait::Task{
		try{
			_exception = nullptr;
			_client = co_await UAClient::GetClient( Connection->Target, Credential{_jwt->Payload()} );//same credential as TokenTests.Authenticate.
		}
		catch( Exception& e ){
			_exception = e.Move();
		}
		flag.test_and_set();
		flag.notify_all();
	}

	TEST_F( TrustReloadTests, ServerReloadsReissuedCert ){
		atomic_flag first;
		Connect( first );
		first.wait( false );
		ASSERT_FALSE( _exception ) << _exception->what();//startup snapshot trusts the pre-created cert (tests/main.cpp).
		ASSERT_TRUE( _client );
		UAClient::RemoveClient( move(_client) );//the next connect builds a fresh UAClient => full OPN handshake.

		//in-place re-issue: same SAN+key, new serial/validity => a DER the server's snapshot has never seen.
		Crypto::IssueCertificate( UAClient::CryptoSettings(Connection->Target, Connection->CertificateUri) );

		atomic_flag second;
		Connect( second );//no server restart - the verify shim must rescan trustedCertDirs and trust the new file.
		second.wait( false );
		EXPECT_FALSE( _exception ) << _exception->what();
		EXPECT_TRUE( _client );
	}

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
		//the bad cert must live OUTSIDE trustedCertDirs: the server rescans them on a failed verify (UATrust), so a
		//cert dropped into an anchored dir is trusted by design - the old ssl-dir swap put it exactly there.
		let root = Process::AppDataFolder();
		auto& ssl = *AppClient()->SslSettings;
		auto bad = ssl;
		bad.Certificate.Path = root/"ssl_badTest"/"certs"/bad.Certificate.Path.filename();
		bad.PrivateKey.Path = root/"ssl_badTest"/"private"/bad.PrivateKey.Path.filename();
		bad.PublicKey.Path = root/"ssl_badTest"/"public"/bad.PublicKey.Path.filename();
		Crypto::EnsureKeyCertificate( bad );//no-op when a previous run left the tree behind.
		struct Restore final{ //the real settings have to come back even when the body throws - otherwise every later test connects with the bad cert.
			Crypto::CryptoSettings Good; Crypto::CryptoSettings& Live;
			~Restore(){ Live = move(Good); }
		} restore{ ssl, ssl };
		ssl = move( bad );

		atomic_flag flag;
		Connect( flag, 'a' );
		flag.wait( false );

		EXPECT_TRUE( _exception );
		EXPECT_TRUE( _exception && string{_exception->what()}.contains("BadSecurityChecksFailed") );
		EXPECT_FALSE( _client );
		DBG( "{}", _exception ? _exception->what() : "Error no exception." );
	}
}
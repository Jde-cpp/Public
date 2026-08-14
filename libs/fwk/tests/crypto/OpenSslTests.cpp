#include <jde/fwk/crypto/OpenSsl.h>
#include "../../src/crypto/OpenSslInternal.h"
#include <jde/fwk/settings.h>
#include <fstream>

#define let const auto
namespace Jde::Crypto{
	constexpr ELogTags _tags = ELogTags::Test;
	using namespace Crypto::Internal;

	struct OpenSslTests : public ::testing::Test{
	protected:
		OpenSslTests() {}
		~OpenSslTests() override{}

		static α SetUpTestCase()->void;
		α SetUp()->void override{};
		α TearDown()->void override{}

		static α GetModulusExponent( fs::path publicKey )ε->tuple<vector<unsigned char>,vector<unsigned char>>;
		static α SslSettings( str publicKeyFile, str privateKeyFile, str certificateFile, sv commonName, sv subjectAltName="URI:urn:my.server.application" )ε->CryptoSettings;

		static string HeaderPayload;
		static string passcode;
		static string PublicKeyFile;
		static string PrivateKeyFile;
		static string CertificateFile;
	};
	string OpenSslTests::HeaderPayload{ "secret stuff" };
	string OpenSslTests::passcode{ "123456789" };
	string OpenSslTests::PublicKeyFile{ _msvc ? (Process::AppDataFolder() / "public.pem").string() : "/tmp/public.pem" };
	string OpenSslTests::PrivateKeyFile{ _msvc ? (Process::AppDataFolder() / "private.pem").string() : "/tmp/private.pem" };
	string OpenSslTests::CertificateFile{ _msvc ? (Process::AppDataFolder() / "cert.pem").string() : "/tmp/cert.pem" };


	α OpenSslTests::SslSettings( str publicKeyFile, str privateKeyFile, str certificateFile, sv commonName, sv subjectAltName )ε->CryptoSettings{
		return CryptoSettings{ jobject{
			{"certificate", jobject{{"path", certificateFile}, {"subjectAltName", subjectAltName}, {"company", "jde-cpp"}, {"country", "US"}, {"commonName", commonName}}},
			{"privateKey", jobject{{"path", privateKeyFile}, {"passcode", passcode}}},
			{"publicKey", jobject{{"path", publicKeyFile}}}
		}, {} };
	}

	α OpenSslTests::SetUpTestCase()->void{
		let clear = Settings::FindBool( "/cryptoTests/clear" ).value_or( true );
		INFO( "clear={}", clear );
		INFO( "HeaderPayload={}", HeaderPayload );
		let settings = SslSettings( PublicKeyFile, PrivateKeyFile, CertificateFile, "openSslTests" );//the CN is the identity target - never "localhost".
		if( clear || (!fs::exists(PublicKeyFile) || !fs::exists(PrivateKeyFile)) ){
			if( !fs::exists(fs::path{PublicKeyFile}.parent_path()) )
				fs::create_directories( fs::path{PublicKeyFile}.parent_path() );
			Crypto::CreateKey( settings, SRCE_CUR );
			INFO( "Created keys {} {}", PublicKeyFile, PrivateKeyFile );
		}
		if( clear || !fs::exists(CertificateFile) ){
			Crypto::IssueCertificate( settings );
			INFO( "Created certificate {}", CertificateFile );
		}
	}

	TEST_F( OpenSslTests, Main ){
		let signature = Crypto::RsaSign( HeaderPayload, PrivateKeyFile, passcode );
		auto publicKey = Crypto::ReadPublicKey( PublicKeyFile );

		Crypto::Verify( publicKey, HeaderPayload, signature );
	}

	//a mismatch is an expected auth outcome, not a malfunction, and EVP_VerifyFinal reports both through one return
	//value - 0 for the first, negative for the second.  The rejected client only ever sees this message, so it has to
	//name the reason rather than echo a bare openssl rc.
	TEST_F( OpenSslTests, VerifyRejectsBadSignature ){
		let signature = Crypto::RsaSign( HeaderPayload, PrivateKeyFile, passcode );
		auto publicKey = Crypto::ReadPublicKey( PublicKeyFile );
		try{
			Crypto::Verify( publicKey, HeaderPayload+"!", signature );//right key, wrong payload.
			FAIL() << "Verify should have thrown.";
		}
		catch( const Exception& e ){
			EXPECT_NE( string{e.what()}.find("Signature verification failed."), string::npos ) << e.what();
		}
	}

	TEST_F( OpenSslTests, Certificate ){
		auto bytes = ReadCertificate( CertificateFile );
		ExtractPublicKey( bytes, SRCE_CUR );
	}
	TEST_F( OpenSslTests, ExtractInfo ){
		let publicKeyFile = _msvc ? (Process::AppDataFolder()/"extractInfo-public.pem").string() : "/tmp/extractInfo-public.pem";
		let privateKeyFile = _msvc ? (Process::AppDataFolder()/"extractInfo-private.pem").string() : "/tmp/extractInfo-private.pem";
		let certificateFile = _msvc ? (Process::AppDataFolder()/"extractInfo-cert.pem").string() : "/tmp/extractInfo-cert.pem";
		Crypto::CreateKeyCertificate( SslSettings(publicKeyFile, privateKeyFile, certificateFile, "extract-info-cn", "email:tester@jde-cpp.com,otherName:1.3.6.1.4.1.311.20.2.3;UTF8:upn-tester@jde-cpp.com,URI:urn:my.server.application") );
		let info = Crypto::Certificate{ ReadCertificate(certificateFile) };
		EXPECT_EQ( info.CommonName, "extract-info-cn" );
		EXPECT_EQ( info.Email, "tester@jde-cpp.com" );
		EXPECT_EQ( info.Upn, "upn-tester@jde-cpp.com" );
		EXPECT_EQ( info.DistinguishedName, "CN=extract-info-cn,O=jde-cpp,C=US" );
		EXPECT_EQ( info.Issuer, info.DistinguishedName );//self-signed.
		//the SAN must come back in the openssl config syntax it was issued with, so a parsed cert can be re-issued.
		EXPECT_EQ( info.SubjectAltName, "email:tester@jde-cpp.com,otherName:msUPN;UTF8:upn-tester@jde-cpp.com,URI:urn:my.server.application" );
		EXPECT_EQ( info.SanUri(), "urn:my.server.application" );//not the whole SAN - that was the applicationUri bug.
		EXPECT_GT( info.Expiration, Clock::now() );//CreateCertificate issues 365-day certs.
		let plain = Crypto::Certificate{ ReadCertificate(CertificateFile) };//fixture cert has a URI-only SAN.
		EXPECT_TRUE( plain.Email.empty() );
		EXPECT_TRUE( plain.Upn.empty() );
		EXPECT_EQ( plain.SubjectAltName, "URI:"+plain.SanUri() );//single entry - SanUri is the whole thing bar the prefix.
	}

	//the CN is the enrollment identity (access_identities.target, a unique natural key) so it must stay per-host, but
	//it is also the file stem - a hostname change would move the key pair, mint a new one and strand the old identity.
	TEST_F( OpenSslTests, FileStemDecouplesPathsFromCommonName ){
		let sslDir = Process::ProgramDataFolder()/Process::CompanyRootDir()/Process::ProductName()/"ssl";
		let names = []( sv fileName, sv commonName ){
			return jobject{ {"certificate", jobject{{"fileName", string{fileName}}, {"commonName", string{commonName}}}} };
		};
		let hostA = CryptoSettings{ names("gateway.web", "gateway.web.hostA") };
		let hostB = CryptoSettings{ names("gateway.web", "gateway.web.hostB") };

		EXPECT_EQ( hostA.PrivateKey.Path, hostB.PrivateKey.Path );//the key survives a rename - same modulus, same user.
		EXPECT_EQ( hostA.PublicKey.Path, hostB.PublicKey.Path );
		EXPECT_EQ( hostA.Certificate.Path, hostB.Certificate.Path );
		EXPECT_EQ( hostA.Certificate.Path, sslDir/"certs"/"gateway.web.pem" );
		EXPECT_NE( hostA.Certificate.CommonName, hostB.Certificate.CommonName );//identity still per-host.

		//no fileName - the CN remains the stem, so every other config is unaffected.
		let legacy = CryptoSettings{ jobject{ {"certificate", jobject{{"commonName", "legacy-cn"}}} } };
		EXPECT_EQ( legacy.Certificate.Path, sslDir/"certs"/"legacy-cn.pem" );
		EXPECT_EQ( legacy.PrivateKey.Path, sslDir/"private"/"legacy-cn.pem" );
	}
	//`productName` at the ssl level names the per-product tree for every path derived from the block.  It used to be
	//honored only by `dh`: sibling servers with different ssl-level productNames shared one cert file derived from
	//Process::ProductName(), while dh alone pointed at the configured tree.  A sub-object's own productName still
	//outranks the block's - the soak's issuedCerts block sets all three explicitly and must keep resolving as written.
	TEST_F( OpenSslTests, SslLevelProductNameNamesTheTree ){
		let companyDir = Process::ProgramDataFolder()/Process::CompanyRootDir();
		let a = CryptoSettings{ jobject{ {"productName", "ProductA"}, {"certificate", jobject{{"commonName", "product-tree-cn"}}} } };
		EXPECT_EQ( a.Certificate.Path, companyDir/"ProductA"/"ssl"/"certs"/"product-tree-cn.pem" );
		EXPECT_EQ( a.PrivateKey.Path, companyDir/"ProductA"/"ssl"/"private"/"product-tree-cn.pem" );
		EXPECT_EQ( a.PublicKey.Path, companyDir/"ProductA"/"ssl"/"public"/"product-tree-cn.pem" );
		EXPECT_EQ( a.DhPath, companyDir/"ProductA"/"ssl"/"dh.pem" );//dh always honored the ssl level - now consistent with its siblings.

		let overridden = CryptoSettings{ jobject{ {"productName", "ProductA"}, {"certificate", jobject{{"commonName", "product-tree-cn"}, {"productName", "ProductB"}}} } };
		EXPECT_EQ( overridden.Certificate.Path, companyDir/"ProductB"/"ssl"/"certs"/"product-tree-cn.pem" );//own wins.
		EXPECT_EQ( overridden.PrivateKey.Path, companyDir/"ProductA"/"ssl"/"private"/"product-tree-cn.pem" );//only the certificate overrode.

		let plain = CryptoSettings{ jobject{ {"certificate", jobject{{"commonName", "product-tree-cn"}}} } };
		EXPECT_EQ( plain.Certificate.Path, companyDir/Process::ProductName()/"ssl"/"certs"/"product-tree-cn.pem" );//no productName anywhere - the process default, unchanged.
	}

	//an unconfigured subjectAltName defaults to the localhost pair - a generated server cert without one is guaranteed
	//to fail host_name_verification, and the hand-spelled per-config line kept getting forgotten (finding: a fresh
	//deployment could never establish trust) or misspelled.  Explicit "" still opts out, any configured value wins.
	TEST_F( OpenSslTests, SubjectAltNameDefaultsToLocalhost ){
		let defaulted = CryptoSettings{ jobject{ {"certificate", jobject{{"commonName", "san-default-cn"}}} } };
		EXPECT_EQ( defaulted.Certificate.SubjectAltName, "DNS:localhost,IP:127.0.0.1" );
		let optedOut = CryptoSettings{ jobject{ {"certificate", jobject{{"commonName", "san-default-cn"}, {"subjectAltName", ""}}} } };
		EXPECT_TRUE( optedOut.Certificate.SubjectAltName.empty() );
		let overridden = CryptoSettings{ jobject{ {"certificate", jobject{{"commonName", "san-default-cn"}, {"subjectAltName", "URI:urn:x"}}} } };
		EXPECT_EQ( overridden.Certificate.SubjectAltName, "URI:urn:x" );
	}

	//certInstance is the OPC Target, settable through the createServerConnection mutation, and the CN is the stem of
	//all three files - neither may escape the ssl tree, or CreateDirectories/IssueCertificate write a PEM anywhere the
	//service account can reach.
	TEST_F( OpenSslTests, PathComponentsCannotEscapeTheSslTree ){
		let sslDir = Process::ProgramDataFolder()/Process::CompanyRootDir()/Process::ProductName()/"ssl";
		let withCn = []( sv commonName ){ return jobject{ {"certificate", jobject{{"commonName", string{commonName}}}} }; };

		let benign = CryptoSettings{ withCn("escape-test"), "TestServer" };//the normal shape is unchanged.
		EXPECT_EQ( benign.Certificate.Path, sslDir/"certs"/"escape-test.TestServer.pem" );

		let evilTarget = CryptoSettings{ withCn("escape-test"), "../../../../etc/cron.d/x" };
		EXPECT_EQ( evilTarget.Certificate.Path.parent_path(), sslDir/"certs" );

		let evilCn = CryptoSettings{ withCn("../../../../etc/cron.d/y") };
		EXPECT_EQ( evilCn.Certificate.Path.parent_path(), sslDir/"certs" );
		EXPECT_EQ( evilCn.PrivateKey.Path.parent_path(), sslDir/"private" );
		EXPECT_EQ( evilCn.PublicKey.Path.parent_path(), sslDir/"public" );
		EXPECT_EQ( evilCn.Certificate.CommonName, "../../../../etc/cron.d/y" );//the CN itself stays intact - it is the X.509 subject and users.target.
	}
	//key+cert are a unit: losing the key must re-issue the certificate, never leave the old one standing against a new
	//key.  A surviving cert would advertise a public key the new private key cannot sign for - and since access_users
	//is keyed by modulus, the process would also enroll as a second identity.
	TEST_F( OpenSslTests, EnsureKeyCertificate_MissingKeyReissuesCert ){
		let dir = ( _msvc ? Process::AppDataFolder() : fs::path{"/tmp"} )/"ensureKeyCert";
		fs::remove_all( dir );
		let settings = SslSettings( (dir/"public.pem").string(), (dir/"private.pem").string(), (dir/"cert.pem").string(), "ensure-key-cert" );
		settings.CreateDirectories();
		Crypto::CreateKeyCertificate( settings );
		auto originalDer = ReadCertificate( settings.Certificate.Path );//lvalue - ExtractPublicKey takes a mutable span.
		let original = Crypto::ExtractPublicKey( originalDer, SRCE_CUR );

		fs::remove( settings.PrivateKey.Path );//damaged install: the key is gone but the certificate survives.
		Crypto::EnsureKeyCertificate( settings );

		auto reissuedDer = ReadCertificate( settings.Certificate.Path );
		let reissued = Crypto::ExtractPublicKey( reissuedDer, SRCE_CUR );
		EXPECT_FALSE( reissued==original );//the stale cert was replaced, not kept.
		EXPECT_TRUE( reissued==Crypto::ReadPublicKey(settings.PublicKey.Path) );//and it matches the key actually on disk.
		fs::remove_all( dir );
	}
	//the other half of MissingKeyReissuesCert, and the only reissueReason branch with no coverage: losing just the
	//certificate must re-issue it on the key already on disk.  Minting a fresh pair here would strand the enrollment
	//identity - access_users is keyed by the modulus, so the process would come back as a stranger to a db still
	//holding the old key, and every peer that anchored the old cert would have to re-trust it.
	TEST_F( OpenSslTests, EnsureKeyCertificate_MissingCertReissuesOnSameKey ){
		let dir = ( _msvc ? Process::AppDataFolder() : fs::path{"/tmp"} )/"missingCert";
		fs::remove_all( dir );
		let settings = SslSettings( (dir/"public.pem").string(), (dir/"private.pem").string(), (dir/"cert.pem").string(), "missing-cert" );
		settings.CreateDirectories();
		Crypto::CreateKeyCertificate( settings );
		let originalKey = Crypto::ReadPublicKey( settings.PublicKey.Path );
		let originalDer = ReadCertificate( settings.Certificate.Path );

		fs::remove( settings.Certificate.Path );//damaged install: the certificate is gone, the key pair survives.
		Crypto::EnsureKeyCertificate( settings );

		ASSERT_TRUE( fs::exists(settings.Certificate.Path) );
		auto reissuedDer = ReadCertificate( settings.Certificate.Path );//lvalue - ExtractPublicKey takes a mutable span.
		EXPECT_FALSE( reissuedDer==originalDer );//genuinely re-issued: IssueCertificate draws a random 128-bit serial.
		EXPECT_TRUE( Crypto::ExtractPublicKey(reissuedDer, SRCE_CUR)==originalKey );//...but on the key that was already there.
		EXPECT_TRUE( Crypto::ReadPublicKey(settings.PublicKey.Path)==originalKey );//and the pair itself was left alone.
		fs::remove_all( dir );
	}
	//an unreadable certificate is a damaged install, not something to paper over by minting a replacement.
	TEST_F( OpenSslTests, EnsureKeyCertificate_BadCertThrows ){
		let dir = ( _msvc ? Process::AppDataFolder() : fs::path{"/tmp"} )/"badCert";
		fs::remove_all( dir );
		let settings = SslSettings( (dir/"public.pem").string(), (dir/"private.pem").string(), (dir/"cert.pem").string(), "bad-cert" );
		settings.CreateDirectories();
		Crypto::CreateKeyCertificate( settings );
		{ std::ofstream truncated{ settings.Certificate.Path, std::ios::trunc }; truncated << "-----BEGIN CERTIFICATE-----\ngarbage\n"; }

		EXPECT_THROW( Crypto::EnsureKeyCertificate(settings), Exception );
		EXPECT_TRUE( fs::exists(settings.PrivateKey.Path) );//the key is left alone, so deleting the cert re-issues on the same modulus.
		fs::remove_all( dir );
	}
	//a cert issued before its config gained a SAN (or with a different one) must heal on startup - the CI runner kept a
	//SAN-less web cert failing host_name_verification until its config dir was hand-wiped.  The key pair must survive
	//the re-issue, and an equivalent config must leave the cert untouched - including conf-syntax slack and the msUPN
	//OID→short-name round-trip, either of which would otherwise re-issue on every start.
	TEST_F( OpenSslTests, EnsureKeyCertificate_ReconcilesSan ){
		let dir = ( _msvc ? Process::AppDataFolder() : fs::path{"/tmp"} )/"reconcileSan";
		fs::remove_all( dir );
		let publicFile = (dir/"public.pem").string(), privateFile = (dir/"private.pem").string(), certFile = (dir/"cert.pem").string();
		let sanless = SslSettings( publicFile, privateFile, certFile, "reconcile-san", "" );
		sanless.CreateDirectories();
		Crypto::CreateKeyCertificate( sanless );
		EXPECT_TRUE( Crypto::Certificate{ ReadCertificate(certFile) }.SubjectAltName.empty() );

		let configured = SslSettings( publicFile, privateFile, certFile, "reconcile-san", "DNS:localhost,IP:127.0.0.1,otherName:1.3.6.1.4.1.311.20.2.3;UTF8:upn@jde-cpp.com" );
		Crypto::EnsureKeyCertificate( configured );//stale environment: the config gained a SAN after the cert was issued.
		auto healedDer = ReadCertificate( certFile );
		EXPECT_EQ( Crypto::Certificate{ healedDer }.SubjectAltName, "DNS:localhost,IP:127.0.0.1,otherName:msUPN;UTF8:upn@jde-cpp.com" );
		EXPECT_TRUE( Crypto::ExtractPublicKey(healedDer, SRCE_CUR)==Crypto::ReadPublicKey(publicFile) );//re-issued on the same key - enrollment by modulus survives.

		let slack = SslSettings( publicFile, privateFile, certFile, "reconcile-san", " DNS:localhost , IP:127.0.0.1 ,otherName:1.3.6.1.4.1.311.20.2.3;UTF8:upn@jde-cpp.com" );
		Crypto::EnsureKeyCertificate( slack );
		EXPECT_TRUE( ReadCertificate(certFile)==healedDer );//equivalent config - the cert stands, no re-issue loop.
		fs::remove_all( dir );
	}
	TEST_F( OpenSslTests, PrivateKey ){
		Crypto::ReadPrivateKey( PrivateKeySettings{PrivateKeyFile, passcode} );
		//the key was created with a passcode - it must be encrypted at rest, i.e. unreadable without it.
		EXPECT_THROW( Crypto::ReadPrivateKey(PrivateKeySettings{PrivateKeyFile, string{}}), Exception );
	}
	TEST_F( OpenSslTests, Random ){
		array<unsigned char,16> a{}, b{};
		Crypto::Random( a.data(), a.size() );
		Crypto::Random( b.data(), b.size() );
		EXPECT_NE( a, b );//2⁻¹²⁸ false-failure odds.
		EXPECT_NE( a, (array<unsigned char,16>{}) );
		Crypto::Random<uint32_t>();
	}
}
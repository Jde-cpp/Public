﻿#include <jde/crypto/OpenSsl.h>
#include "../src/OpenSslInternal.h"
//#include "../../src/crypto/OpenSslInternal.h"
#include <jde/framework/settings.h>

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

		static string HeaderPayload;
		static string passcode;
		static string PublicKeyFile;
		static string PrivateKeyFile;
		static string CertificateFile;
	};
	string OpenSslTests::HeaderPayload{ "secret stuff" };
	string OpenSslTests::passcode{ "123456789" };
	string OpenSslTests::PublicKeyFile{ _msvc ? (OSApp::ApplicationDataFolder() / "public.pem").string() : "/tmp/public.pem" };
	string OpenSslTests::PrivateKeyFile{ _msvc ? (OSApp::ApplicationDataFolder() / "private.pem").string() : "/tmp/private.pem" };
	string OpenSslTests::CertificateFile{ _msvc ? (OSApp::ApplicationDataFolder() / "cert.pem").string() : "/tmp/cert.pem" };


	α OpenSslTests::SetUpTestCase()->void{
		let clear = Settings::FindBool( "cryptoTests/clear" ).value_or( true );
		Information( _tags, "clear={}", clear );
		Information( _tags, "clear={}", HeaderPayload );
		if( clear || (!fs::exists(PublicKeyFile) || !fs::exists(PrivateKeyFile)) ){
			if( !fs::exists(fs::path{PublicKeyFile}.parent_path()) )
				fs::create_directories( fs::path{PublicKeyFile}.parent_path() );
			Crypto::CreateKey( PublicKeyFile, PrivateKeyFile, passcode );
			Information( _tags, "Created keys {} {}", PublicKeyFile, PrivateKeyFile );
		}
		if( clear || !fs::exists(CertificateFile) ){
			Crypto::CreateCertificate( CertificateFile, PrivateKeyFile, passcode, "URI:urn:my.server.application", "jde-cpp", "US", "localhost" );
			Information( _tags, "Created certificate {}", CertificateFile );
		}
	}

	TEST_F( OpenSslTests, Main ){
		let signature = Crypto::RsaSign( HeaderPayload, PrivateKeyFile );
		auto [modulus2, exponent2] = Crypto::ModulusExponent( PublicKeyFile );

		Crypto::Verify( modulus2, exponent2, HeaderPayload, signature );
	}

	TEST_F( OpenSslTests, Certificate ){
		ReadCertificate( CertificateFile );
	}
	TEST_F( OpenSslTests, PrivateKey ){
		Crypto::ReadPrivateKey( PrivateKeyFile, {} );
	}
}
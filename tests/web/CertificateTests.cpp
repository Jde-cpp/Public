//#include <execution>
#include <jde/web/flex/Flex.h>
#include <jde/crypto/OpenSsl.h>
#include "mocks/ServerMock.h"
#include "../../../Ssl/source/Ssl.h"
// #include "../../../Framework/source/Stopwatch.h"
// #include "TestRequestAwait.h"

#define var const auto
namespace Jde::Web{
	using Mock::Host;
	using Mock::Port;

	static sp<Jde::LogTag> _logTag{ Logging::Tag( "tests" ) };
	using CryptoSettings = Crypto::CryptoSettings;
	struct CertificateTests : public ::testing::Test{
	protected:
		CertificateTests(){}
		~CertificateTests() override{}

		static α SetUpTestCase()->void{};
		α SetUp()->void override{};
		α TearDown()->void override;
		static json OriginalSettings;
	};
	json CertificateTests::OriginalSettings{ Settings::Global().Json() };

	α ResetSettings( const fs::path& baseDir = {} )ι->void{
		//sv what, const Container::Variant& v, bool save=true, SRCE )ε->void;
		var prefix = "http/ssl"s;
		Settings::Set( prefix+"/certificate", baseDir/"certs/server.pem", false );
		Settings::Set( prefix+"/privateKey", baseDir/"private/server.pem", false );
		Settings::Set( prefix+"/publicKey", baseDir/"public/server.pem", false );
	}

	α CertificateTests::TearDown()->void{
		Mock::Stop();
		Settings::Global().Json() = OriginalSettings;
	}

	TEST_F( CertificateTests, DefaultSettings ){
		ResetSettings( IApplication::ApplicationDataFolder()/"ssl" );
		Mock::Start();
		var result = Ssl::Send<string>( Host, "/isSsl", {}, std::to_string(Port), "text/ping", {}, http::verb::get );
		ASSERT_EQ( "SSL=true", result );
	}

	TEST_F( CertificateTests, NewDirectory ){
		ResetSettings( "/tmp/WebTests/ssl" );
		fs::remove_all( "/tmp/WebTests/ssl" );
		Settings::Set( "http/ssl/passcode", "PaSsCoDe", false );
		Mock::Start();
		var result = Ssl::Send<string>( Host, "/isSsl", {}, std::to_string(Port), "text/ping", {}, http::verb::get );
		ASSERT_EQ( "SSL=true", result );
	}
}
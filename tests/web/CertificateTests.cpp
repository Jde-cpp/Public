#include <jde/web/client/http/ClientHttpAwait.h>
#include <jde/web/server/Flex.h>
#include <jde/crypto/OpenSsl.h>
#include "mocks/ServerMock.h"


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

		Ω SetUpTestCase()->void{};
		α SetUp()->void override{ Mock::Start(); }
		α TearDown()->void override;
		static json OriginalSettings;
	};
	json CertificateTests::OriginalSettings = Settings::Global().Json();

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
	using Web::Client::ClientHttpAwait;
	using Web::Client::ClientHttpRes;
	TEST_F( CertificateTests, DefaultSettings ){
		ResetSettings( IApplication::ApplicationDataFolder()/"ssl" );
		auto await = ClientHttpAwait{ Host, "/isSsl", Port, {.ContentType="text/ping"} };
		var res = BlockAwait<ClientHttpAwait,ClientHttpRes>( move(await) );
		ASSERT_EQ( "SSL=true", res.Body() );
	}

	TEST_F( CertificateTests, NewDirectory ){
		ResetSettings( "/tmp/WebTests/ssl" );
		fs::remove_all( "/tmp/WebTests/ssl" );
		Settings::Set( "http/ssl/passcode", "PaSsCoDe", false );
		auto await = ClientHttpAwait{ Host, "/isSsl", Port, {.ContentType="text/ping"} };
		var res = BlockAwait<ClientHttpAwait,ClientHttpRes>( move(await) );
		ASSERT_EQ( "SSL=true", res.Body() );
	}
}
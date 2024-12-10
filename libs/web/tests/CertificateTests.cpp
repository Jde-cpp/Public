#include <jde/web/client/http/ClientHttpAwait.h>
//#include <jde/web/server/Flex.h>
#include <jde/crypto/OpenSsl.h>
#include "mocks/ServerMock.h"


#define let const auto
namespace Jde::Web{
	using Mock::Host;
	using Mock::Port;

	static sp<Jde::LogTag> _logTag{ Logging::Tag( "test" ) };
	using CryptoSettings = Crypto::CryptoSettings;
	struct CertificateTests : public ::testing::Test{
	protected:
		CertificateTests(){}
		~CertificateTests() override{}

		Ω SetUpTestCase()->void{};
		α SetUp()->void override{ Mock::Start(); }
		α TearDown()->void override;
	};

	α ResetSettings( const fs::path& baseDir = {} )ι->void{
		//sv what, const Container::Variant& v, bool save=true, SRCE )ε->void;
		let prefix = "/http/ssl"s;
		Settings::Set( prefix+"/certificate", jvalue{(baseDir/"certs/server.pem").string()} );
		Settings::Set( prefix+"/privateKey", jvalue{(baseDir/"private/server.pem").string()} );
		Settings::Set( prefix+"/publicKey", jvalue{(baseDir/"public/server.pem").string()} );
	}

	α CertificateTests::TearDown()->void{
		Mock::Stop();
		Settings::Load();
	}
	using Web::Client::ClientHttpAwait;
	using Web::Client::ClientHttpRes;
	TEST_F( CertificateTests, DefaultSettings ){
		ResetSettings( IApplication::ApplicationDataFolder()/"ssl" );
		auto await = ClientHttpAwait{ Host, "/ping", Port, {.ContentType="text/ping", .Verb=http::verb::post} };
		let res = BlockAwait<ClientHttpAwait,ClientHttpRes>( move(await) );
		ASSERT_TRUE( res[http::field::server].contains("SSL") );
	}

	TEST_F( CertificateTests, NewDirectory ){
		ResetSettings( "/tmp/WebTests/ssl" );
		fs::remove_all( "/tmp/WebTests/ssl" );
		Settings::Set( "http/ssl/passcode", jvalue{"PaSsCoDe"} );
		auto await = ClientHttpAwait{ Host, "/ping", Port, {.ContentType="text/ping", .Verb=http::verb::post} };
		let res = BlockAwait<ClientHttpAwait,ClientHttpRes>( move(await) );
		ASSERT_TRUE( res[http::field::server].contains("SSL") );
	}
}
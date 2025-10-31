#include <jde/web/client/http/ClientHttpAwait.h>
//#include <jde/web/server/Flex.h>
#include <jde/fwk/crypto/OpenSsl.h>
#include "mocks/ServerMock.h"


#define let const auto
namespace Jde::Web{
	using Mock::Host;
	using Mock::Port;

	using CryptoSettings = Crypto::CryptoSettings;
	struct CertificateTests : public ::testing::Test{
	protected:
		CertificateTests(){}
		~CertificateTests() override{}

		Ω SetUpTestCase()->void{};
		α SetUp()->void override{}
		α TearDown()->void override;
	};

	α SslSettings( const fs::path& baseDir = {}, string passcode = {} )ι->jobject{
		//sv what, const Container::Variant& v, bool save=true, SRCE )ε->void;
		let prefix = "/http/ssl"s;
		return jobject{
			{"port", Port},
			{"ssl", jobject{
				{"certificate", (baseDir/"certs/server.pem").string()},
				{"privateKey", (baseDir/"private/server.pem").string()},
				{"publicKey", (baseDir/"public/server.pem").string()},
				{"passcode", passcode}
			}}
		};
	}

	α CertificateTests::TearDown()->void{
		Mock::Stop();
		Settings::Load();
	}
	using Web::Client::ClientHttpAwait;
	using Web::Client::ClientHttpRes;
	TEST_F( CertificateTests, DefaultSettings ){
		Mock::Start( SslSettings(Process::ApplicationDataFolder()/"ssl") );
		auto await = ClientHttpAwait{ Host, "/ping", Port, {.ContentType="text/ping", .Verb=http::verb::post} };
		let res = BlockAwait<ClientHttpAwait,ClientHttpRes>( move(await) );
		ASSERT_TRUE( res[http::field::server].contains("SSL") );
	}

	TEST_F( CertificateTests, NewDirectory ){
		let path = Settings::FindPath( "testing/certDir" ).value_or( fs::temp_directory_path()/"webTests/ssl" );
		fs::remove_all( path );
		Mock::Start( SslSettings(path, "PaSsCoDe") );
		auto await = ClientHttpAwait{ Host, "/ping", Port, {.ContentType="text/ping", .Verb=http::verb::post} };
		let res = BlockAwait<ClientHttpAwait,ClientHttpRes>( move(await) );
		ASSERT_TRUE( res[http::field::server].contains("SSL") );
	}
}
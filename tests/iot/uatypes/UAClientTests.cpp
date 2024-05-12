#include <jde/iot/types/OpcServer.h>
#include <jde/iot/uatypes/UAClient.h>
#include <jde/crypto/OpenSsl.h>

#include <open62541/plugin/create_certificate.h>

#define var const auto

namespace Jde::Iot{
	static sp<Jde::LogTag> _logTag{ Logging::Tag( "tests" ) };

	struct UAClientTests : public ::testing::Test{
	protected:
		UAClientTests()ι{}
		~UAClientTests()ι override{}

		static α SetUpTestCase()ι->void{};
		α SetUp()ι->void override{};
		α TearDown()ι->void override{}
	};

	std::condition_variable_any cv;
	std::shared_mutex mtx;
	sp<UAClient> _pClient;
	α Connect()ι->Task{
		try{
			_pClient = ( co_await UAClient::GetClient(Settings::Get("iot/target").value_or("")) ).SP<UAClient>();
		}
		catch( const IException& ){}
		std::shared_lock l{ mtx };
		cv.notify_one();
	}
	α CreateCertificate()ι->void{
		UA_ByteString derPrivKey = UA_BYTESTRING_NULL;
		UA_ByteString derCert = UA_BYTESTRING_NULL;
		UA_String subject[3] = {UA_STRING_STATIC("C=DE"),
			UA_STRING_STATIC("O=SampleOrganization"),
			UA_STRING_STATIC("CN=Open62541Server@localhost")};
		UA_UInt32 lenSubject = 3;
		UA_String subjectAltName[2]= {
			UA_STRING_STATIC("DNS:localhost"),
			//UA_STRING_STATIC("URI:urn:open62541.server.application")
			UA_STRING_STATIC("URI:urn:JDE-CPP:Kepware.KEPServerEX.V6:UA%20Server")
		};
		UA_UInt32 lenSubjectAltName = 2;
		UA_KeyValueMap *kvm = UA_KeyValueMap_new();
		UA_UInt16 expiresIn = 365;
		UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "expires-in-days"),
			(void *)&expiresIn, &UA_TYPES[UA_TYPES_UINT16]);
		UA_UInt16 keyLength = 2048;
		UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "key-size-bits"),
			(void *)&keyLength, &UA_TYPES[UA_TYPES_UINT16]);
		UA_StatusCode status = UA_CreateCertificate(
			UA_Log_Stdout, subject, lenSubject, subjectAltName, lenSubjectAltName,
			UA_CERTIFICATEFORMAT_DER, kvm, &derPrivKey, &derCert);
		var root{ IApplication::ApplicationDataFolder() };
		Crypto::WriteCertificate( root/"uacert.pem", vector<byte>{(byte*)derCert.data, (byte*)derCert.data+derCert.length} );
		Crypto::WritePrivateKey( root/"uaprivate.pem", vector<byte>{(byte*)derPrivKey.data, (byte*)derPrivKey.data+derPrivKey.length}, OSApp::EnvironmentVariable("JDE_PASSCODE") );
		//openssl pkcs12 -export -in cert.pem -inkey private.pem -out private.pfx
	}

	TEST_F( UAClientTests, ConnectSecure ){
		INFO( "UAClientTests.Main" );
		//CreateCertificate();
		Connect();
		std::shared_lock l{ mtx };
		cv.wait( l );
		//EXPECT_TRUE( not_nullptr(_pClient) );
		EXPECT_TRUE(_pClient);

		INFO( "~UAClientTests.Main" );
	}

}
#include <jde/iot/UM.h>
#include <jde/iot/types/OpcServer.h>
#include <jde/iot/uatypes/UAClient.h>
#include <jde/crypto/OpenSsl.h>

#include <open62541/plugin/create_certificate.h>
#include "../helpers.h"

#define var const auto

namespace Jde::Iot{
	static sp<Jde::LogTag> _logTag{ Logging::Tag( "tests" ) };

	struct UAClientTests : public ::testing::Test{
	protected:
		UAClientTests()ι{}
		~UAClientTests()override{}

		Ω SetUpTestCase()ι->void;
		α SetUp()ι->void override{};
		α TearDown()ι->void override{}
	public:
		static json OpcServer;
	};
	json UAClientTests::OpcServer{};

	α UAClientTests::SetUpTestCase()ι->void{
		OpcServer = Iot::SelectOpcServer();
		if( OpcServer.empty() ){
			uint id = Iot::CreateOpcServer();
			OpcServer = Iot::SelectOpcServer( id );
		}
	}

	std::condition_variable_any cv;
	std::shared_mutex mtx;
	sp<UAClient> _pClient;
	vector<SessionPK> _sessionIds;
	up<IException> _exception;
	α AuthenticateTest( bool badPassword=false )ι->Task{
		try{
			var sessionId = *( co_await Iot::Authenticate("user1", badPassword ? "xyz" :"password1", UAClientTests::OpcServer["target"]) ).UP<SessionPK>();
			_sessionIds.push_back( sessionId );
			//_pClient = ( co_await UAClient::GetClient("user1", badPassword ? "xyz" :"password1") ).SP<UAClient>();
		}
		catch( IException& e ){
			_exception = e.Move();
		}
		std::shared_lock l{ mtx };
		cv.notify_one();
	}
/*	α CreateCertificate()ι->void{
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
*/
	TEST_F( UAClientTests, Authenticate ){
		INFO( "UAClientTests.Authenticate" );
		AuthenticateTest();
		AuthenticateTest();
		{
			std::shared_lock l{ mtx };
			cv.wait( l );
			cv.wait( l );
		}
		AuthenticateTest();
		std::shared_lock l{ mtx };
		cv.wait( l );
		var creds = Credentials( _sessionIds[2], OpcServer["target"] );
		EXPECT_EQ( "user1", get<0>(creds) );
		EXPECT_EQ( "password1", get<1>(creds) );
		EXPECT_NE( _sessionIds[0], _sessionIds[1] );
		EXPECT_NE( _sessionIds[0], _sessionIds[2] );
		EXPECT_NE( _sessionIds[1], _sessionIds[2] );
		EXPECT_TRUE( find(_sessionIds, 0)==_sessionIds.end() );
		INFO( "~UAClientTests.Authenticate" );
	}
	
	TEST_F( UAClientTests, Authenticate_BadPassword ){
		INFO( "UAClientTests.Authenticate_BadPassword" );
		AuthenticateTest( true );
		std::shared_lock l{ mtx };
		cv.wait( l );
		//EXPECT_FALSE( _pClient );
		EXPECT_TRUE( _exception );
		EXPECT_TRUE( string{_exception->what()}.contains("BadUserAccessDenied") );
		DBG( "{}", _exception->what() );
		INFO( "~UAClientTests.Authenticate_BadPassword" );
	}
}
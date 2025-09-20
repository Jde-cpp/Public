#include "UAConfig.h"
#include <open62541/server_config_default.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/certificategroup_default.h>
#include <jde/crypto/OpenSsl.h>
#include <jde/app/client/IAppClient.h>

#define let const auto
namespace Jde::Opc::Server {
	UAConfig::UAConfig()ε:
		UA_ServerConfig{
			.logging{ &_logger },
		}{
		if( auto certificateFile = Settings::FindPath("/opcServer/ssl/certificate").value_or(fs::path{}); !certificateFile.empty() ){
			try{
				SetupSecurityPolicies( move(certificateFile) );
			}
			catch( std::exception& e ){
				UA_ServerConfig_clear( this );
				throw move(e);
			}
		}
		else
			UA_ServerConfig_setDefault( this );

/*		for( uint i=0; i<config.securityPoliciesSize; ++i ){
			let& sp = config.securityPolicies[i];
			Information{ ELogTags::App, "[{}]PolicyUri={}", i, ToSV(sp.policyUri) };
			if( ToSV(sp.policyUri)=="http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256" )
				UA_AccessControl_default( &config, false/ *allowAnonymous* /, &sp.policyUri, usernamePasswordsSize, usernamePasswords );
		}*/
/*	applicationDescription.applicationUri = UA_STRING_NULL;
		applicationDescription.productUri = UA_STRING("OpcServer");
		applicationDescription.applicationName = UA_LOCALIZEDTEXT("en-US", "OpcServer");
		applicationDescription.applicationType = UA_APPLICATIONTYPE_SERVER;
*/
	}

	α UAConfig::SetupSecurityPolicies( fs::path&& certificateFile )ε->void{
		let passcode = Settings::FindString("/opcServer/ssl/privateKey/passcode").value_or("");
		auto privateKeyFile = Settings::FindPath( "/opcServer/ssl/privateKey/path" ).value_or( fs::path{} );
		if( !fs::exists(certificateFile) ){
			let parentPath = certificateFile.parent_path();
			fs::create_directories( parentPath );
			fs::create_directories( privateKeyFile.parent_path() );
			Crypto::CreateKey( parentPath/"public.pem", privateKeyFile, passcode );
			const string uri{ "urn:open62541.server.application" };
			Crypto::CreateCertificate( certificateFile, privateKeyFile, passcode, Ƒ("URI:{}", uri), "jde-cpp", "US", "localhost" );
		}
		auto certificate = ToUAByteString( Crypto::ReadCertificate(certificateFile) );
		auto privateKey = ToUAByteString( Crypto::ReadPrivateKey(privateKeyFile, passcode) );
		SetConfig( Settings::FindNumber<PortType>("/opcServer/port").value_or(4840), move(certificate), move(privateKey) );
//		UA_ServerConfig_setDefaultWithSecurityPolicies( &config, Settings::FindNumber<PortType>("/tcp/port").value_or(4840), certificate.get(), privateKey.get(), &trustList, 0, &issuerList, 0, &revocationList, 0 );
		UA_String_clear( &applicationDescription.applicationUri );
		applicationDescription.applicationUri = UA_STRING_ALLOC("urn:open62541.server.application");
	}

	α UAConfig::SetConfig( PortType port, ByteStringPtr&& certificate, const ByteStringPtr&& privateKey )ε->void{
    UAε( UA_ServerConfig_setBasics_withPort(this, port) );

		vector<UA_ByteString> trustedCerts;
		for( let& sdir : Settings::FindStringArray("/opcServer/trustedCertDirs") ){
			let dir = fs::path{ sdir };
			if( !fs::exists(dir) || !fs::is_directory(dir) )
				continue;
			for( let& entry : fs::directory_iterator(dir) ){
				if( entry.path().extension()==".pem" || entry.path().extension()==".crt" )
					trustedCerts.push_back( *ToUAByteString(Crypto::ReadCertificate({entry.path()})).release() );
			}
		}
		UA_ByteString issuerList; uint issuerListSize = 0;
		UA_ByteString revocationList; uint revocationListSize = 0;
    UA_TrustListDataType list;
    UA_TrustListDataType_init(&list);
    if( trustedCerts.size() ){
			list.specifiedLists |= UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
			UAε( UA_Array_copy(&trustedCerts[0], trustedCerts.size(), (void**)&list.trustedCertificates, &UA_TYPES[UA_TYPES_BYTESTRING]) );
			list.trustedCertificatesSize = trustedCerts.size();
    }
    if(issuerListSize > 0) {
			list.specifiedLists |= UA_TRUSTLISTMASKS_ISSUERCERTIFICATES;
			UAε( UA_Array_copy(&issuerList, issuerListSize, (void**)&list.issuerCertificates, &UA_TYPES[UA_TYPES_BYTESTRING]) );
			list.issuerCertificatesSize = issuerListSize;
    }
    if(revocationListSize > 0) {
			list.specifiedLists |= UA_TRUSTLISTMASKS_TRUSTEDCRLS;
			UAε( UA_Array_copy(&revocationList, revocationListSize, (void**)&list.trustedCrls, &UA_TYPES[UA_TYPES_BYTESTRING]) );
			list.trustedCrlsSize = revocationListSize;
    }

    /* Set up the parameters */
    UA_KeyValuePair params[2];
    size_t paramsSize = 2;

    params[0].key = UA_QualifiedName{ 0, "max-trust-listsize"_uv };
    UA_Variant_setScalar(&params[0].value, &maxTrustListSize, &UA_TYPES[UA_TYPES_UINT32]);
    params[1].key = UA_QualifiedName{ 0, "max-rejected-listsize"_uv };
    UA_Variant_setScalar(&params[1].value, &maxRejectedListSize, &UA_TYPES[UA_TYPES_UINT32]);

    UA_KeyValueMap paramsMap;
    paramsMap.map = params;
    paramsMap.mapSize = paramsSize;

    if( secureChannelPKI.clear )
        secureChannelPKI.clear( &secureChannelPKI );
    UA_NodeId defaultApplicationGroup = UA_NODEID_NUMERIC( 0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP );
		try{
    	UAε( UA_CertificateGroup_Memorystore(&secureChannelPKI, &defaultApplicationGroup, &list, logging, &paramsMap) );
		  if( sessionPKI.clear )
        sessionPKI.clear( &sessionPKI );
    	UA_NodeId defaultUserTokenGroup = UA_NODEID_NUMERIC( 0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP );
    	UAε( UA_CertificateGroup_Memorystore( &sessionPKI, &defaultUserTokenGroup, &list, logging, &paramsMap) );
		}
		catch( std::exception& e ){
			UA_TrustListDataType_clear(&list);
			throw move(e);
		}
    UA_TrustListDataType_clear(&list);
		AddSecurityPolicies( move(certificate), move(privateKey) );

		UAAccess::Init( *this );

    UAε( UA_ServerConfig_addAllEndpoints(this) );
	}

	α UAConfig::AddSecurityPolicies( ByteStringPtr&& certificate, const ByteStringPtr&& privateKey )ε->void{
    UA_ByteString localCertificate = *certificate;
    UA_ByteString localPrivateKey  = *privateKey;

    // Load the private key and convert to the DER format. Use an empty password on the first try -- maybe the key does not require a password.
    UA_ByteString decryptedPrivateKey = UA_BYTESTRING_NULL;
    UA_ByteString keyPassword = UA_BYTESTRING_NULL;
    if (privateKey && privateKey->length > 0)
        UAε( UA_CertificateUtils_decryptPrivateKey(localPrivateKey, keyPassword, &decryptedPrivateKey) );
    /* Basic256Sha256 */
    UAε( UA_ServerConfig_addSecurityPolicyBasic256Sha256(this, &localCertificate,&decryptedPrivateKey) );

    //UAε( UA_ServerConfig_addSecurityPolicyAes256Sha256RsaPss(this, &localCertificate, &decryptedPrivateKey) );
    //UAε( UA_ServerConfig_addSecurityPolicyAes128Sha256RsaOaep(this, &localCertificate, &decryptedPrivateKey) );
    UAε( UA_ServerConfig_addSecurityPolicyNone(this, &localCertificate) );
    //UAε( UA_ServerConfig_addSecurityPolicyEccNistP256(this, &localCertificate, &decryptedPrivateKey);
    UA_ByteString_memZero( &decryptedPrivateKey );
    UA_ByteString_clear( &decryptedPrivateKey );
	}

	α UAConfig::SetAccessControl()ι{

	}
	constexpr uint usernamePasswordsSize = 2;
	UA_UsernamePasswordLogin usernamePasswords[usernamePasswordsSize] = {
    {UA_STRING_STATIC("user1"), UA_STRING_STATIC("0123456789ABCD")},
    {UA_STRING_STATIC("user2"), UA_STRING_STATIC("0123456789ABCD")}};
}
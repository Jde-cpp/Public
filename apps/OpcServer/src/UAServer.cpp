#include "UAServer.h"
#include <open62541/server_config_default.h>
#include <open62541/plugin/accesscontrol_default.h>

#include <jde/crypto/OpenSsl.h>
#include <jde/opc/uatypes/helpers.h>


#define let const auto
namespace Jde::Opc::Server {
	Ω getConfiguration()->UA_ServerConfig;
	UAServer::UAServer()ε:
		ServerName{ Settings::FindString("/opcServer/name").value_or("OpcServer") },
		_config{ getConfiguration() },
		_ua{ UA_Server_newWithConfig(&_config) }
	{}
	UAServer::~UAServer(){
		Information{ ELogTags::App, "Stopping OPC UA server..." };
		if( _thread.has_value() )
			_thread->request_stop();
		if( _ua ){
			UA_Server_delete( _ua );
			_ua = nullptr;
		}
	}
	α UAServer::Run()ι->void{
		_thread = std::jthread{[this](std::stop_token st){
    	UA_Server_runUntilInterrupt( _ua );
    	UA_Server_delete( _ua );
		}};
	}

	α setupSecurityPolicies( UA_ServerConfig& config, fs::path&& certificateFile )ε->void{
		let passcode = Settings::FindString("/tcp/privateKey/passcode").value_or("");
		auto privateKeyFile = Settings::FindPath( "/tcp/privateKey/path" ).value_or( fs::path{} );
		if( !fs::exists(certificateFile) ){
			let parentPath = certificateFile.parent_path();
			Crypto::CreateKey( certificateFile.parent_path()/"public.pem", privateKeyFile, passcode );
			const string uri{ "urn:open62541.server.application" };
			Crypto::CreateCertificate( certificateFile, privateKeyFile, passcode, Jde::format("URI:{}", uri), "jde-cpp", "US", "localhost" );
		}
		auto certificate = ToUAByteString( Crypto::ReadCertificate(certificateFile) );
		auto privateKey = ToUAByteString( Crypto::ReadPrivateKey(privateKeyFile, passcode) );
		UA_ByteString trustList;
		UA_ByteString issuerList;
		UA_ByteString revocationList;
		UA_ServerConfig_setDefaultWithSecurityPolicies( &config, Settings::FindNumber<UA_UInt16>("/tcp/port").value_or(4840), certificate.get(), privateKey.get(), &trustList, 0, &issuerList, 0, &revocationList, 0 );
		UA_String_clear(&config.applicationDescription.applicationUri);
		config.applicationDescription.applicationUri = UA_STRING_ALLOC("urn:open62541.server.application");
	}

	constexpr uint usernamePasswordsSize = 2;
	UA_UsernamePasswordLogin usernamePasswords[usernamePasswordsSize] = {
    {UA_STRING_STATIC("user1"), UA_STRING_STATIC("0123456789ABCD")},
    {UA_STRING_STATIC("user2"), UA_STRING_STATIC("0123456789ABCD")}};

	α getConfiguration()->UA_ServerConfig{
		UA_ServerConfig config{};
		//config.logging = &_logger; TODO
		if( auto certificateFile = Settings::FindPath("/tcp/certificate").value_or(fs::path{}); !certificateFile.empty() )
			setupSecurityPolicies( config, move(certificateFile) );
		else
			UA_ServerConfig_setDefault(&config);

		for( uint i=0; i<config.securityPoliciesSize; ++i ){
			let& sp = config.securityPolicies[i];
			Information{ ELogTags::App, "[{}]PolicyUri={}", i, ToSV(sp.policyUri) };
			if( ToSV(sp.policyUri)=="http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256" )
				UA_AccessControl_default( &config, false/*allowAnonymous*/, &sp.policyUri, usernamePasswordsSize, usernamePasswords );
		}
		return config;
	}
}
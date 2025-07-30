#include "UAAccess.h"
#include <open62541/plugin/accesscontrol_default.h>
#include <jde/app/client/IAppClient.h>
#include "UAConfig.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"

#define let const auto

typedef struct {
    UA_Boolean allowAnonymous;
    size_t usernamePasswordLoginSize;
    UA_UsernamePasswordLogin *usernamePasswordLogin;
    UA_UsernamePasswordLoginCallback loginCallback;
    void *loginContext;
    UA_CertificateGroup verifyX509;

		bool AllowCertificate;
		bool AllowIssued;
		UA_String UserTokenPolicyUri;
} AccessControlContext;

namespace Jde::Opc::Server{
	struct SessionContext final{
		SessionContext( jobject&& o )ε:
			SessionId{ Json::AsNumber<SessionPK>(o, "sessionId") },
			UserPK{ Json::AsNumber<UserPK::Type>(o, "userPK") },
			Expiration{ Json::AsTimePoint(o, "expiration") },
			Endpoint{ Json::AsString(o, "endpoint") }
		{}

		SessionPK SessionId;
		UserPK UserPK;
		TimePoint Expiration;
		string Endpoint;
	};
}

namespace Jde::Opc::Server::UAAccess{
	Ω setContext( UA_AccessControl& ac )ι->AccessControlContext&{
    auto cntxt = (AccessControlContext*)UA_malloc( sizeof(AccessControlContext) );
    memset( cntxt, 0, sizeof(AccessControlContext) );
    cntxt->allowAnonymous = Settings::FindBool("/opc/tokenTypes/anonymous").value_or(false);
		cntxt->AllowCertificate = Settings::FindBool("/opc/tokenTypes/certificate").value_or(true);
		cntxt->AllowIssued = Settings::FindBool("/opc/tokenTypes/issued").value_or(true);
		cntxt->UserTokenPolicyUri = AllocUAString( Settings::FindString("/opc/userTokenPolicyUri").value_or("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256") );
		ac.context = cntxt;
		return *cntxt;
	}

	Ω assignFunctions( UA_AccessControl& ac )ι{
			// .context{ nullptr },
			// .clear{ [](UA_AccessControl *ac){ if( ac && ac->context ) UA_free(ac->context); } },
			// .userTokenPoliciesSize{ 0 },
			// .userTokenPolicies{ nullptr },
		ac.activateSession = &ActivateSession;
		ac.closeSession = &CloseSession;
		ac.getUserRightsMask = &GetUserRightsMask;
		ac.getUserAccessLevel = &GetUserAccessLevel;
		ac.getUserExecutable = &GetUserExecutable;
		ac.getUserExecutableOnObject = &GetUserExecutableOnObject;
		ac.allowAddNode = &AllowAddNode;
		ac.allowAddReference = &AllowAddReference;
		ac.allowDeleteNode = &AllowDeleteNode;
		ac.allowDeleteReference = &AllowDeleteReference;
		ac.allowBrowseNode = &AllowBrowseNode;
		ac.allowTransferSubscription = &AllowTransferSubscription;
		ac.allowHistoryUpdateUpdateData = &AllowHistoryUpdateUpdateData;
		ac.allowHistoryUpdateDeleteRawModified = &AllowHistoryUpdateDeleteRawModified;
	}
}
namespace Jde::Opc::Server{
	α UAAccess::Init( UAConfig& config )ε->void{
		auto& ac = config.accessControl;
		assignFunctions( ac );
		auto& context = setContext( ac );
    let numOfPolicies = context.UserTokenPolicyUri.length ? 1 : config.securityPoliciesSize;
    uint policies{}; string log{};
    if( context.allowAnonymous ){
			log += "Anonymous,";
      ++policies;
		}
    if( context.AllowCertificate ){
			log += "Certificate,";
      ++policies;
		}
    if( context.AllowIssued ){
			log += "IssuedToken,";
      ++policies;
		}
		THROW_IF( !policies, "No allowed policies set." );
		log.pop_back();
		Information{ (ELogTags)EOpcLogTags::Server, "UserToken Uris:  [{}]", move(log) };
		log.clear();

		ac.userTokenPoliciesSize = policies * numOfPolicies;
    ac.userTokenPolicies = (UA_UserTokenPolicy*)UA_Array_new( ac.userTokenPoliciesSize, &UA_TYPES[UA_TYPES_USERTOKENPOLICY] );
    policies = 0;
    for( uint i = 0; i < numOfPolicies; ++i ){
			const UA_String utpUri = context.UserTokenPolicyUri.length ? context.UserTokenPolicyUri : config.securityPolicies[i].policyUri;
			log += ToString(utpUri) + ",";
      if( context.allowAnonymous ){
      	ac.userTokenPolicies[policies].tokenType = UA_USERTOKENTYPE_ANONYMOUS;
        ac.userTokenPolicies[policies].policyId = ToUV( "open62541-anonymous-policy" );
        UA_String_copy( &utpUri, &ac.userTokenPolicies[policies].securityPolicyUri );
        ++policies;
      }
      if( context.AllowCertificate ){
        ac.userTokenPolicies[policies].tokenType = UA_USERTOKENTYPE_CERTIFICATE;
        ac.userTokenPolicies[policies].policyId = ToUV( "open62541-certificate-policy" );
        if( UA_String_equal(&utpUri, &UA_SECURITY_POLICY_NONE_URI) )
					Debug{ (ELogTags)EOpcLogTags::Server, "x509 Certificate Authentication configured, but no encrypting SecurityPolicy. This can leak credentials on the network." };
        UA_String_copy( &utpUri, &ac.userTokenPolicies[policies].securityPolicyUri );
				++policies;
			}
      if( context.AllowIssued ){
        ac.userTokenPolicies[policies].tokenType = UA_USERTOKENTYPE_ISSUEDTOKEN;
        ac.userTokenPolicies[policies].policyId = UA_STRING_ALLOC("open62541-issuedtoken-policy");
        if( UA_String_equal(&utpUri, &UA_SECURITY_POLICY_NONE_URI) )
					Debug{ (ELogTags)EOpcLogTags::Server, "IssuedToken Authentication configured, but no encrypting SecurityPolicy. This can leak credentials on the network." };
				UA_String_copy( &utpUri, &ac.userTokenPolicies[policies].securityPolicyUri );
				++policies;
      }
    }
		log.pop_back();
		Information{ (ELogTags)EOpcLogTags::Server, "UserToken Uris:  [{}]", move(log) };
	}
	α UAAccess::ActivateSession( UA_Server *server, UA_AccessControl *ac, const UA_EndpointDescription *endpointDescription, const UA_ByteString *secureChannelRemoteCertificate, const UA_NodeId *sessionId, const UA_ExtensionObject *userIdentityToken, void **sessionContext )ι->UA_StatusCode{
		const UA_String anonymous_policy = "open62541-anonymous-policy"_uv;
		const UA_String certificate_policy = "open62541-certificate-policy"_uv;
		const UA_String username_policy = "open62541-username-policy"_uv;

    AccessControlContext *context = (AccessControlContext*)ac->context;
    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* The empty token is interpreted as anonymous */
    UA_AnonymousIdentityToken anonToken;
    UA_ExtensionObject tmpIdentity;
    if(userIdentityToken->encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY) {
        UA_AnonymousIdentityToken_init(&anonToken);
        UA_ExtensionObject_init(&tmpIdentity);
        UA_ExtensionObject_setValueNoDelete(&tmpIdentity,
                                            &anonToken,
                                            &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN]);
        userIdentityToken = &tmpIdentity;
    }

    /* Could the token be decoded? */
    if(userIdentityToken->encoding < UA_EXTENSIONOBJECT_DECODED)
        return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

    const UA_DataType *tokenType = userIdentityToken->content.decoded.type;
    if(tokenType == &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN]) {
        /* Anonymous login */
        if(!context->allowAnonymous)
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

        const UA_AnonymousIdentityToken *token = (UA_AnonymousIdentityToken*)
            userIdentityToken->content.decoded.data;

        /* Match the beginnig of the PolicyId.
         * Compatibility notice: Siemens OPC Scout v10 provides an empty
         * policyId. This is not compliant. For compatibility, assume that empty
         * policyId == ANONYMOUS_POLICY */
        if(token->policyId.data &&
           (token->policyId.length < anonymous_policy.length ||
            strncmp((const char*)token->policyId.data,
                    (const char*)anonymous_policy.data,
                    anonymous_policy.length) != 0)) {
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;
        }
    } else if(tokenType == &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN]) {
			/* Username and password */
			const UA_UserNameIdentityToken *userToken = (UA_UserNameIdentityToken*)
					userIdentityToken->content.decoded.data;

			/* Match the beginnig of the PolicyId */
			if(userToken->policyId.length < username_policy.length ||
					strncmp((const char*)userToken->policyId.data,
									(const char*)username_policy.data,
									username_policy.length) != 0) {
					return UA_STATUSCODE_BADIDENTITYTOKENINVALID;
			}

			/* The userToken has been decrypted by the server before forwarding
				* it to the plugin. This information can be used here. */
			/* if(userToken->encryptionAlgorithm.length > 0) {} */

			/* Empty username and password */
			if(userToken->userName.length == 0 && userToken->password.length == 0)
					return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

			/* Try to match username/pw */
			UA_Boolean match = false;
			if(context->loginCallback) {
					if(context->loginCallback(&userToken->userName, &userToken->password,
							context->usernamePasswordLoginSize, context->usernamePasswordLogin,
							sessionContext, context->loginContext) == UA_STATUSCODE_GOOD)
							match = true;
			} else {
					for(size_t i = 0; i < context->usernamePasswordLoginSize; i++) {
							if(UA_String_equal(&userToken->userName, &context->usernamePasswordLogin[i].username) &&
									UA_ByteString_equal(&userToken->password, &context->usernamePasswordLogin[i].password)) {
									match = true;
									break;
							}
					}
			}
			if(!match)
					return UA_STATUSCODE_BADUSERACCESSDENIED;
    } else if(tokenType == &UA_TYPES[UA_TYPES_X509IDENTITYTOKEN]) {
			const UA_X509IdentityToken *userToken = (UA_X509IdentityToken*)userIdentityToken->content.decoded.data;
			try{
				if(userToken->policyId.length < certificate_policy.length ||
					strncmp((const char*)userToken->policyId.data,
									(const char*)certificate_policy.data,
									certificate_policy.length) != 0) {
						throw UAException{ UA_STATUSCODE_BADIDENTITYTOKENINVALID };
				}
				THROW_IFX( !config->sessionPKI.verifyCertificate, UAException{UA_STATUSCODE_BADIDENTITYTOKENINVALID} );
				UAε( config->sessionPKI.verifyCertificate(&config->sessionPKI, &userToken->certificateData) );
				auto publicKey = Crypto::ExtractPublicKey( std::span<byte>{(byte*)userToken->certificateData.data, userToken->certificateData.length} );
				let exp = publicKey.ExponentInt();
				auto user = AppClient()->QuerySync( Ƒ("user( modulus: \"{}\", exponent: {} ){{target name}}", publicKey.ModulusHex(), exp) );
				THROW_IF( user.empty(), "Certificate user not found: modulus: {}, exponent: {}", publicKey.ModulusHex(), exp );
				auto jwt = mu<Web::Jwt>( move(publicKey), Json::AsString(user, "name"), Json::AsString(user, "target") );
				*sessionContext = jwt.release();
			}
			catch( const UAException& e ){
				return e.Code;
			}
			catch( const exception& e ){
				return UA_STATUSCODE_BADIDENTITYTOKENINVALID;
			}
    }
		else if(tokenType == &UA_TYPES[UA_TYPES_ISSUEDIDENTITYTOKEN]) {
      const UA_IssuedIdentityToken* userToken = (UA_IssuedIdentityToken*)userIdentityToken->content.decoded.data;
			try{
				Web::Jwt jwt{ ToSV(userToken->tokenData) };
				AppClient()->Verify( jwt );
				*sessionContext = new Web::Jwt{ move(jwt) };
				//auto info = AppClient()->QuerySyncSecure( Ƒ("session(jwt: {}){{expiration sessionId userPK endpoint}}", ToString(userToken->tokenData)) );
			}
			catch( exception& e ){
				return UA_STATUSCODE_BADIDENTITYTOKENINVALID;
			}
    }
		 else {
        /* Unsupported token type */
        return UA_STATUSCODE_BADIDENTITYTOKENINVALID;
    }

    return UA_STATUSCODE_GOOD;
	}
	α UAAccess::CloseSession(UA_Server* server, UA_AccessControl* ac,const UA_NodeId* sessionId, void* sessionContext)ι->void{
		Web::Jwt* jwt = static_cast<Web::Jwt*>( sessionContext );
		delete jwt;
	}
	α UAAccess::GetUserRightsMask(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *nodeId, void *nodeContext)ι->UA_UInt32{ ASSERT(false); return false; }
	α UAAccess::GetUserAccessLevel( UA_Server* server, UA_AccessControl* ac, const UA_NodeId* sessionId, void* sessionContext, const UA_NodeId* nodeId, void* nodeContext )ι->UA_Byte{
		ASSERT( nodeId );
		if( !nodeId )
			return false;
		let id = nodeId->identifier.numeric;
		ASSERT( nodeId->identifierType == UA_NODEIDTYPE_NUMERIC && nodeId->namespaceIndex == 0 );
		if( id == UA_NS0ID_SERVER_NAMESPACEARRAY ){
			return UA_ACCESSLEVELMASK_READ;
		}
		Warning{ (ELogTags)EOpcLogTags::Server, "GetUserAccessLevel: nodeId {} not enforced", id };
		BREAK;
		return std::numeric_limits<UA_Byte>::max();
	}
	α UAAccess::GetUserExecutable(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *methodId, void *methodContext)ι->UA_Boolean{
		ASSERT( methodId );
		if( !methodId )
			return false;
		ASSERT( methodId->identifierType == UA_NODEIDTYPE_NUMERIC && methodId->namespaceIndex == 0 );
		let id = methodId->identifier.numeric;
		if( id == UA_NS0ID_SERVER_NAMESPACEARRAY ){
			return true;
		}
		Warning{ (ELogTags)EOpcLogTags::Server, "GetUserExecutable: MethodId {} not enforced", id };
		BREAK;
		return true;
	}
	α UAAccess::GetUserExecutableOnObject(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *methodId, void *methodContext, const UA_NodeId *objectId, void *objectContext)ι->UA_Boolean{ ASSERT(false); return false; }
	α UAAccess::AllowAddNode(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_AddNodesItem *item)ι->UA_Boolean{ ASSERT(false); return false; }
	α UAAccess::AllowAddReference(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_AddReferencesItem *item)ι->UA_Boolean{ ASSERT(false); return false; }
	α UAAccess::AllowDeleteNode(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_DeleteNodesItem *item)ι->UA_Boolean{ ASSERT(false); return false; }
	α UAAccess::AllowDeleteReference(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_DeleteReferencesItem *item)ι->UA_Boolean{ ASSERT(false); return false; }
	α UAAccess::AllowBrowseNode(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *nodeId, void *nodeContext)ι->UA_Boolean{ ASSERT(false); return false; }
	α UAAccess::AllowTransferSubscription(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *oldSessionId, void *oldSessionContext, const UA_NodeId *newSessionId, void *newSessionContext)ι->UA_Boolean{ ASSERT(false); return false; }
	α UAAccess::AllowHistoryUpdateUpdateData(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *nodeId, UA_PerformUpdateType performInsertReplace, const UA_DataValue *value)ι->UA_Boolean{ ASSERT(false); return false; }
	α UAAccess::AllowHistoryUpdateDeleteRawModified(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *nodeId, UA_DateTime startTimestamp, UA_DateTime endTimestamp, bool isDeleteModified)ι->UA_Boolean{ ASSERT(false); return false; }
}
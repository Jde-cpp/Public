#include "UAAccess.h"
#include <open62541/plugin/accesscontrol_default.h>
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
	α UAAccess::ActivateSession( UA_Server *server, UA_AccessControl *ac, const UA_EndpointDescription *endpointDescription, const UA_ByteString *secureChannelRemoteCertificate, const UA_NodeId *sessionId, const UA_ExtensionObject *userIdentityToken, void **sessionContext )ι->UA_StatusCode{ ASSERT(false); return UA_STATUSCODE_BADNOTIMPLEMENTED; }
	α UAAccess::CloseSession(UA_Server *server, UA_AccessControl *ac,const UA_NodeId *sessionId, void *sessionContext)ι->void{ ASSERT(false); }
	α UAAccess::GetUserRightsMask(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *nodeId, void *nodeContext)ι->UA_UInt32{ ASSERT(false); return false; }
	α UAAccess::GetUserAccessLevel(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *nodeId, void *nodeContext)ι->UA_Byte{ ASSERT(false); return false; }
	α UAAccess::GetUserExecutable(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *methodId, void *methodContext)ι->UA_Boolean{ ASSERT(false); return false; }
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
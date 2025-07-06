#include "UAAccess.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"

namespace Jde::Opc::Server{
	α UAAccess::activateSession( UA_Server *server, UA_AccessControl *ac, const UA_EndpointDescription *endpointDescription, const UA_ByteString *secureChannelRemoteCertificate, const UA_NodeId *sessionId, const UA_ExtensionObject *userIdentityToken, void **sessionContext )ι->UA_StatusCode{ ASSERT(false); return UA_STATUSCODE_BADNOTIMPLEMENTED; }
	α UAAccess::closeSession(UA_Server *server, UA_AccessControl *ac,const UA_NodeId *sessionId, void *sessionContext)ι->void{ ASSERT(false); }
	α UAAccess::getUserRightsMask(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *nodeId, void *nodeContext)ι->UA_UInt32{ ASSERT(false); return false; }
	α UAAccess::getUserAccessLevel(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *nodeId, void *nodeContext)ι->UA_Byte{ ASSERT(false); return false; }
	α UAAccess::getUserExecutable(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *methodId, void *methodContext)ι->UA_Boolean{ ASSERT(false); return false; }
	α UAAccess::getUserExecutableOnObject(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *methodId, void *methodContext, const UA_NodeId *objectId, void *objectContext)ι->UA_Boolean{ ASSERT(false); return false; }
	α UAAccess::allowAddNode(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_AddNodesItem *item)ι->UA_Boolean{ ASSERT(false); return false; }
	α UAAccess::allowAddReference(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_AddReferencesItem *item)ι->UA_Boolean{ ASSERT(false); return false; }
	α UAAccess::allowDeleteNode(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_DeleteNodesItem *item)ι->UA_Boolean{ ASSERT(false); return false; }
	α UAAccess::allowDeleteReference(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_DeleteReferencesItem *item)ι->UA_Boolean{ ASSERT(false); return false; }
	α UAAccess::allowBrowseNode(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *nodeId, void *nodeContext)ι->UA_Boolean{ ASSERT(false); return false; }
	α UAAccess::allowTransferSubscription(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *oldSessionId, void *oldSessionContext, const UA_NodeId *newSessionId, void *newSessionContext)ι->UA_Boolean{ ASSERT(false); return false; }
	α UAAccess::allowHistoryUpdateUpdateData(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *nodeId, UA_PerformUpdateType performInsertReplace, const UA_DataValue *value)ι->UA_Boolean{ ASSERT(false); return false; }
	α UAAccess::allowHistoryUpdateDeleteRawModified(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *nodeId, UA_DateTime startTimestamp, UA_DateTime endTimestamp, bool isDeleteModified)ι->UA_Boolean{ ASSERT(false); return false; }
}
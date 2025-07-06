#pragma once

namespace Jde::Opc::Server{
	struct UAAccess :	UA_AccessControl{
    α activateSession( UA_Server *server, UA_AccessControl *ac, const UA_EndpointDescription *endpointDescription, const UA_ByteString *secureChannelRemoteCertificate, const UA_NodeId *sessionId, const UA_ExtensionObject *userIdentityToken, void **sessionContext )ι->UA_StatusCode;
    α closeSession(UA_Server *server, UA_AccessControl *ac,const UA_NodeId *sessionId, void *sessionContext)ι->void;
    α getUserRightsMask(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *nodeId, void *nodeContext)ι->UA_UInt32;
		α getUserAccessLevel(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *nodeId, void *nodeContext)ι->UA_Byte;
		α getUserExecutable(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *methodId, void *methodContext)ι->UA_Boolean;
		α getUserExecutableOnObject(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *methodId, void *methodContext, const UA_NodeId *objectId, void *objectContext)ι->UA_Boolean;
		α allowAddNode(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_AddNodesItem *item)ι->UA_Boolean;
		α allowAddReference(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_AddReferencesItem *item)ι->UA_Boolean;
		α allowDeleteNode(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_DeleteNodesItem *item)ι->UA_Boolean;
		α allowDeleteReference(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_DeleteReferencesItem *item)ι->UA_Boolean;
		α allowBrowseNode(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *nodeId, void *nodeContext)ι->UA_Boolean;
		α allowTransferSubscription(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *oldSessionId, void *oldSessionContext, const UA_NodeId *newSessionId, void *newSessionContext)ι->UA_Boolean;
		α allowHistoryUpdateUpdateData(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *nodeId, UA_PerformUpdateType performInsertReplace, const UA_DataValue *value)ι->UA_Boolean;
		α allowHistoryUpdateDeleteRawModified(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *nodeId, UA_DateTime startTimestamp, UA_DateTime endTimestamp, bool isDeleteModified)ι->UA_Boolean;
	};
}
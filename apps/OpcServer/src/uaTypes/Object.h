#pragma once
#include "ObjectAttr.h"
#include "BrowseName.h"
#include "Node.h"

namespace Jde::Opc::Server{
	struct Object final: Node, UA_ObjectAttributes{
		Object()ι:UA_ObjectAttributes{}{}
		Object( const jobject& j, Server::NodePK parentPK, Server::BrowseName browse )ε;
		Object( UA_NodeId n )ι;
		Object( NodePK pk, UA_NodeId&& n )ι;
		//Object( Node&& n, ObjectAttr&& attr )ι;
		Object( DB::Row&& r, sp<ObjectType> typeDef )ι;
		α InsertParams()Ι->vector<DB::Value>;

		α Specified()Ι->UA_UInt32 override{ return specifiedAttributes; }
		α Name()Ι->UA_LocalizedText override{ return displayName; }
		α Description()Ι->UA_LocalizedText override{ return description; }
		α WriteMask()Ι->UA_UInt32 override{ return writeMask; }
		α UserWriteMask()Ι->UA_UInt32 override{ return userWriteMask; }
	};
}
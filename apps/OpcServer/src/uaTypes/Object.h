#pragma once
#include <jde/opc/uatypes/BrowseName.h>
#include "ObjectAttr.h"
#include "Node.h"

namespace Jde::Opc::Server{
	struct Object final: Node, UA_ObjectAttributes{
		Object()ι:UA_ObjectAttributes{}{}
		Object( const jobject& j, Server::NodePK parentPK, Opc::BrowseName browse )ε;
		Object( UA_NodeId n )ι;
		Object( NodePK pk, UA_NodeId&& n )ι;
		//Object( Node&& n, ObjectAttr&& attr )ι;
		Object( DB::Row&& r, sp<ObjectType> typeDef )ι;
		Object( const Object& v )ι;
		Object( Object&& v )ι;
		α operator=( const Object& v )ι->Object&;
		α operator=( Object&& v )ι->Object&;
		~Object() override;
		α InsertParams()Ι->vector<DB::Value> override;

		α Specified()Ι->UA_UInt32 override{ return specifiedAttributes; }
		α Name()Ι->UA_LocalizedText override{ return displayName; }
		α Description()Ι->UA_LocalizedText override{ return description; }
		α WriteMask()Ι->UA_UInt32 override{ return writeMask; }
		α UserWriteMask()Ι->UA_UInt32 override{ return userWriteMask; }
	};
}
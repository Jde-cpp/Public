#pragma once
#include "ObjectTypeAttr.h"
#include "BrowseName.h"
#include "Node.h"

namespace Jde::Opc::Server{
	struct ObjectType final: Node, UA_ObjectTypeAttributes{
		ObjectType(): Node{UA_NODEID_NULL}, UA_ObjectTypeAttributes{} {}
		ObjectType( const jobject& j, Server::NodePK parentPK, Server::BrowseName browse )ε;
		ObjectType( UA_NodeId n )ι;
		//ObjectType( Node&& n )ι;
		ObjectType( Node&& n, ObjectTypeAttr&& a )ι;
		ObjectType( DB::Row& r )ι;

		α ToString( const Node& parent )Ι->string override;
		α InsertParams()ι->vector<DB::Value>;
		α IsObjectType()Ι->bool override{ return true; }
		α Specified()Ι->UA_UInt32 override{ return specifiedAttributes; }
		α Name()Ι->UA_LocalizedText override{ return displayName; }
		α Description()Ι->UA_LocalizedText override{ return description; }
		α WriteMask()Ι->UA_UInt32 override{ return writeMask; }
		α UserWriteMask()Ι->UA_UInt32 override{ return userWriteMask; }
	};
}
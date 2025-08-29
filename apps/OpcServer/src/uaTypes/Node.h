#pragma once
#include "ObjectAttr.h"
#include "BrowseName.h"
#include <jde/opc/uatypes/NodeId.h>

namespace Jde::Opc::Server{
	struct ObjectType;
	struct Node : NodeId{
		Node()ι;
		Node( NodeId&& nodeId )ι;
		Node( NodeId nodeId, NodePK pk )ι;
		Node( const jobject& j, NodePK parentPK, BrowseName browse )ε;
		Node( UA_NodeId nodeId, NodePK parentPK, NodePK refPK, sp<ObjectType> typeDef, BrowseName browseName )ι;
		Node( NodePK pk );
		Node( DB::Row&& r, sp<ObjectType> typeDef )ι;

		β IsObjectType()Ι->bool{ return false; }
		α InsertParams()ι->vector<DB::Value>;

		NodePK PK{};
		bool IsGlobal{};
		Server::NodePK ParentNodePK{};
		Server::NodePK ReferenceTypePK{};
		BrowseName Browse;
		sp<ObjectType> TypeDef;
		β ToString( const Node& parent )Ι->string;
		α ToString()Ι->string{ return NodeId::ToString(); }
		β Specified()Ι->UA_UInt32=0;
		α BrowseName()Ι->str{ return Browse.Name; }
		β Name()Ι->UA_LocalizedText=0;
		β Description()Ι->UA_LocalizedText=0;
		β WriteMask()Ι->UA_UInt32=0;
		β UserWriteMask()Ι->UA_UInt32=0;
	};
}
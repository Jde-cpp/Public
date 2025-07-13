#pragma once
#include "ObjectAttr.h"
#include "BrowseName.h"
#include <jde/opc/uatypes/Node.h>

namespace Jde::Opc::Server{
	struct ObjectType;
	struct Node : ExNodeId{
		Node( UA_NodeId nodeId, NodePK pk=0 )ι;
		Node( const jobject& j, NodePK parentPK, BrowseName browse )ε;
		Node( UA_NodeId nodeId, NodePK parentPK, NodePK refPK, sp<ObjectType> typeDef, BrowseName browseName )ι;
		Node( NodePK pk );
		Node( DB::Row&& r, sp<ObjectType> typeDef )ι;

		β IsObjectType()Ι->bool{ return false; }
		α InsertParams( bool extended )ι->vector<DB::Value>;

		NodePK PK{};
		bool IsGlobal{};
		Server::NodePK ParentNodePK{};
		Server::NodePK ReferenceTypePK{};
		BrowseName Browse;
		sp<ObjectType> TypeDef;
		β ToString( const Node& parent )Ι->string;
		β Specified()Ι->UA_UInt32=0;
		α BrowseName()Ι->str{ return Browse.Name; }
		β Name()Ι->UA_LocalizedText=0;
		β Description()Ι->UA_LocalizedText=0;
		β WriteMask()Ι->UA_UInt32=0;
		β UserWriteMask()Ι->UA_UInt32=0;
	};
}
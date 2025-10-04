#pragma once
#include "ObjectAttr.h"
#include "BrowseName.h"
#include "Node.h"

namespace Jde::Opc::Server{
	struct Reference;
	struct Variable final: Node, UA_VariableAttributes{
		Variable():UA_VariableAttributes{}{}
		Variable( const Variable& v )ι;
		Variable( Variable&& v )ι;
		Variable( UA_NodeId n )ι;
		Variable( jobject&& j, NodePK parentPK, Server::BrowseName browse )ι;
		Variable( DB::Row& r, sp<ObjectType> typeDef, UA_Variant&& variant, const UA_DataType& dataType, tuple<UA_UInt32*, uint> dims )ε;

		α operator=( const Variable& v )ι->Variable&;
		α operator=( Variable&& v )ι->Variable&;

		α InsertParams()Ι->vector<DB::Value> override;
		α InsertParams( DB::Value variantPK )Ι->vector<DB::Value>;

		α Specified()Ι->UA_UInt32 override{ return specifiedAttributes; }
		α Name()Ι->UA_LocalizedText override{ return displayName; }
		α Description()Ι->UA_LocalizedText override{ return description; }
		α WriteMask()Ι->UA_UInt32 override{ return writeMask; }
		α UserWriteMask()Ι->UA_UInt32 override{ return userWriteMask; }

		vector<sp<Reference>>	_refs;
	};
}
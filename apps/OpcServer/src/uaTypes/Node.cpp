#include "Node.h"
#include "ObjectType.h"

namespace Jde::Opc::Server{
	Node::Node( UA_NodeId nodeId, NodePK pk )ι:
		NodeId{ move(nodeId) },
		PK{ pk ? pk : IsSystem(nodeId) ? (NodePK)nodeId.identifier.numeric : 0 }
	{}

	Node::Node( UA_NodeId nodeId, NodePK parentPK, NodePK refPK, sp<ObjectType> typeDef, Server::BrowseName browse )ι:
		NodeId{ nodeId },
		PK{},
		ParentNodePK{ parentPK },
		ReferenceTypePK{ refPK },
		Browse{ browse.PK },
		TypeDef{ typeDef }
	{}
/*	Ω getNodeId( const jobject& j )->NodeId{
		auto id = j.if_contains("id");
		return id ? NodeId{ *id } : UA_NodeId{};
	}*/
	Node::Node( const jobject& j, NodePK parentPK, Server::BrowseName browse )ε:
		NodeId{ j.contains("id") ? NodeId{ j.at("id") } : UA_NodeId{} },
		PK{ Json::FindNumber<Server::NodePK>(j, "id").value_or(0) },
		IsGlobal{ Json::FindBool(j, "isGlobal").value_or(false) },
		ParentNodePK{ parentPK },
		ReferenceTypePK{ Json::FindNumberPath<Server::NodePK>(j, "ref/id").value_or(0) },
		Browse{ move(browse) },
		TypeDef{ j.contains("type") ? ms<ObjectType>(NodeId{j.at("type").as_object()}.nodeId) : nullptr }
	{}

	Node::Node( NodePK pk ):
		Node{ pk<=32750 ? UA_NodeId{0, UA_NODEIDTYPE_NUMERIC, (UA_UInt32)pk} : UA_NodeId{}, pk }
	{}

	Node::Node( DB::Row&& r, sp<ObjectType> typeDef )ι:
		NodeId{ r, 1, true },
		PK{ r.GetUInt(0) },
		IsGlobal{ r.GetBitOpt(8).value_or(false) },
		ParentNodePK{ r.GetUInt32Opt(9).value_or(0) },
		ReferenceTypePK{ r.GetUInt32Opt(10).value_or(0) },
		Browse{ r.GetUInt32Opt(11).value_or(0) },
		TypeDef{ typeDef }
	{}

	α Node::InsertParams( bool extended )ι->vector<DB::Value>{
		auto params = NodeId::InsertParams( extended );
		params.emplace_back( ParentNodePK );
		params.emplace_back( ReferenceTypePK );
		if( !IsObjectType() )
			params.emplace_back( TypeDef ? DB::Value{TypeDef->PK} : DB::Value{} );
		params.emplace_back( Browse.PK );
		params.emplace_back( DB::Value{Specified(), {0}} );
		params.emplace_back( Opc::ToString(Name().text) );
		params.emplace_back( Opc::ToString(Description().text), "" );
		params.emplace_back( WriteMask() );
		params.emplace_back( UserWriteMask() );
		return params;
	}
	α Node::ToString( const Node& parent )Ι->string{
		return Ƒ( "[{}]{}.{}", NodeId::to_string(), parent.BrowseName(), BrowseName() );
	}
}
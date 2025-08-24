#include "ObjectType.h"

namespace Jde::Opc::Server{
	ObjectType::ObjectType( const jobject& j, Server::NodePK parentPK, Server::BrowseName browse )ε:
		Node{ j, parentPK, move(browse) },
		UA_ObjectTypeAttributes{ ObjectTypeAttr{j} }
	{}

	ObjectType::ObjectType( UA_NodeId n )ι:
		Node{ move(n) },
		UA_ObjectTypeAttributes{}
	{}

	ObjectType::ObjectType( Node&& n, ObjectTypeAttr&& a )ι:
		Node{ move(n) },
		UA_ObjectTypeAttributes{ move(a) }
	{}
	ObjectType::ObjectType( DB::Row& r )ι:
		Node{ move(r), {} },
		UA_ObjectTypeAttributes{
			r.GetUInt32Opt(12).value_or(0),
			UA_LocalizedText{ "en-US"_uv, AllocUAString(r.GetString(13)) },
			UA_LocalizedText{ "en-US"_uv, AllocUAString(r.GetString(14)) },
			r.GetUInt32Opt(15).value_or(0),
			r.GetUInt32Opt(16).value_or(0),
			r.GetBitOpt(20).value_or(false)
		}
	{}
	α ObjectType::InsertParams()ι->vector<DB::Value>{
		vector<DB::Value> params = Node::InsertParams();
		params.emplace_back( isAbstract, 0 );
		return params;
	}

	α ObjectType::ToString()Ι->string{
		return Ƒ( "[{}]{}", NodeId::ToString(), BrowseName() );
	}
	α ObjectType::ToString( const Node& parent )Ι->string{
		return Node::ToString(parent);
	}
}
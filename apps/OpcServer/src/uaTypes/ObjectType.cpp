#include "ObjectType.h"

namespace Jde::Opc::Server{
	ObjectType::ObjectType( const jobject& j, Server::NodePK parentPK, Opc::BrowseName browse )ε:
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
	ObjectType::ObjectType( const ObjectType& v )ι:
		Node{ v }{
		UA_ObjectTypeAttributes_copy( &v, this );//deep-copy: the implicit copy ctor would share attribute pointers that the destructor would then double-free.
	}
	ObjectType::ObjectType( ObjectType&& v )ι:
		Node{ move(v) },
		UA_ObjectTypeAttributes{ move(v) }{
		UA_ObjectTypeAttributes_init( &v );//we stole v's attributes; keep its destructor from freeing them.
	}
	α ObjectType::operator=( const ObjectType& v )ι->ObjectType&{
		if( this != &v ){
			Node::operator=( v );
			UA_ObjectTypeAttributes_clear( this );
			UA_ObjectTypeAttributes_copy( &v, this );
		}
		return *this;
	}
	α ObjectType::operator=( ObjectType&& v )ι->ObjectType&{
		if( this != &v ){
			Node::operator=( move(v) );
			UA_ObjectTypeAttributes_clear( this );
			memcpy( &this->specifiedAttributes, &v.specifiedAttributes, sizeof(UA_ObjectTypeAttributes) );
			UA_ObjectTypeAttributes_init( &v );
		}
		return *this;
	}
	ObjectType::~ObjectType(){
		UA_ObjectTypeAttributes_clear( this );
	}
	α ObjectType::InsertParams()Ι->vector<DB::Value>{
		vector<DB::Value> params = Node::InsertParams();
		params.emplace_back( isAbstract, 0 );
		return params;
	}

	α ObjectType::ToString()Ι->string{
		return Ƒ( "[{}]{}", NodeId::ToString(), ToSV(BrowseName()) );
	}
	α ObjectType::ToString( const Node& parent )Ι->string{
		return Node::ToString(parent);
	}
}
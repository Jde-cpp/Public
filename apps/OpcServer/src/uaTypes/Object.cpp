#include "Object.h"

#define let const auto

namespace Jde::Opc::Server{
	Object::Object( const jobject& j, Server::NodePK parentPK, Opc::BrowseName browse )ε:
		Node{ j, parentPK, move(browse) },
		UA_ObjectAttributes{
			Json::FindNumber<UA_UInt32>(j, "specified").value_or(0),
			UA_LocalizedText{ "en-US"_uv, AllocUAString(j.at("name").as_string()) },
			UA_LocalizedText{ "en-US"_uv, AllocUAString(Json::FindSV(j, "description").value_or("")) },
			Json::FindNumber<UA_UInt32>( j, "writeMask" ).value_or(0),
			Json::FindNumber<UA_UInt32>( j, "userWriteMask").value_or(0),
			Json::FindNumber<UA_Byte>( j, "eventNotifier" ).value_or(0)
		}
	{}
	Object::Object( UA_NodeId n )ι:
		Node{ NodeId{move(n)} },
		UA_ObjectAttributes{}
	{}
	Object::Object( NodePK pk, UA_NodeId&& n )ι:
		Node{ move(n), pk },
		UA_ObjectAttributes{}
	{}

	Object::Object( DB::Row&& r, sp<ObjectType> typeDef )ι:
		Node{ move(r), typeDef },
		UA_ObjectAttributes{
			r.GetUInt32Opt(13).value_or(0),
			UA_LocalizedText{ "en-US"_uv, AllocUAString(r.GetString(14)) },
			UA_LocalizedText{ "en-US"_uv, AllocUAString(r.GetString(15)) },
			r.GetUInt32Opt(16).value_or(0),
			r.GetUInt32Opt(17).value_or(0),
			r.GetBitOpt(21).value_or(false)
		}
	{}

	Object::Object( const Object& v )ι:
		Node{ v }{
		UA_ObjectAttributes_copy( &v, this );//deep-copy: without an explicit copy ctor the implicit one shares attribute pointers, which the destructor would then double-free.
	}
	Object::Object( Object&& v )ι:
		Node{ move(v) },
		UA_ObjectAttributes{ move(v) }{
		UA_ObjectAttributes_init( &v );//we stole v's attributes; keep its destructor from freeing them.
	}
	α Object::operator=( const Object& v )ι->Object&{
		if( this != &v ){
			Node::operator=( v );
			UA_ObjectAttributes_clear( this );
			UA_ObjectAttributes_copy( &v, this );
		}
		return *this;
	}
	α Object::operator=( Object&& v )ι->Object&{
		if( this != &v ){
			Node::operator=( move(v) );
			UA_ObjectAttributes_clear( this );
			memcpy( &this->specifiedAttributes, &v.specifiedAttributes, sizeof(UA_ObjectAttributes) );
			UA_ObjectAttributes_init( &v );
		}
		return *this;
	}
	Object::~Object(){
		UA_ObjectAttributes_clear( this );
	}
	α Object::InsertParams()Ι->vector<DB::Value>{
		vector<DB::Value> params = Node::InsertParams();
		params.emplace_back( eventNotifier, 0 );
		return params;
	}
}
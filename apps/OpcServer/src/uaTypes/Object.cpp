#include "Object.h"

#define let const auto

namespace Jde::Opc::Server{
	Object::Object( const jobject& j, Server::NodePK parentPK, Server::BrowseName browse )ε:
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
		Node{ n },
		UA_ObjectAttributes{}
	{}
	Object::Object( NodePK pk, UA_NodeId&& n )ι:
		Node{ move(n) },
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

	α Object::InsertParams()ι->vector<DB::Value>{
		vector<DB::Value> params = Node::InsertParams( false );
		params.emplace_back( eventNotifier, 0 );
		return params;
	}
}
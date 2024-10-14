#pragma once
#include "../Exports.h"
#include "Logger.h"

namespace Jde::Iot{
	namespace Browse{ struct Response; }
	struct UAClient;
	struct NodeId;
	struct Variant;

	struct ΓI Value : UA_DataValue{
		Value( StatusCode sc )ι:UA_DataValue{ .status{sc}, .hasStatus{true} }{}
		Value( const UA_DataValue& x )ι{ UA_DataValue_copy( &x, this ); }
		Value( const Value& x )ι{ UA_DataValue_copy( &x, this ); }
		Value( Value&& x )ι:UA_DataValue{x}{ UA_DataValue_init(&x); }
		~Value(){ UA_DataValue_clear(this); }

		α operator=( Value&& x )ι->Value&{ UA_DataValue_copy( &x, this ); return *this; }
		α IsScaler()Ι->bool{ return UA_Variant_isScalar( &value ); }
		α ToProto( const OpcNK& opcId, const NodeId& nodeId )Ι->FromServer::Message;
		α ToJson()Ι->json;
		α Set( const json& j )ε->void;
		Ŧ Get( uint index )Ι->const T&{ return ((T*)value.data)[index]; };
	private:
		Ŧ SetNumber( const json& j )ε->void;
	};

	namespace Read{
		struct ΓI Await final : IAwait{
			Await( flat_set<NodeId>&& x, sp<UAClient>&& c, SRCE )ι;
			α Suspend()ι->void override;
			α await_resume()ι->AwaitResult override{ Trace(IotReadTag, "Read::await_resume"); return IAwait::await_resume(); }
		private:
			flat_set<NodeId> _nodes;
			sp<UAClient> _client;
		};

		Ξ SendRequest( flat_set<NodeId> x, sp<UAClient> c )ι->Await{ return Await{ move(x), move(c) }; }
		α OnResponse( UA_Client *client, void *userdata, RequestId requestId, StatusCode status, UA_DataValue *let )ι->void;
	}

	Ŧ Value::SetNumber( const json& j )ε->void{
		if( UA_Variant_isScalar(&value) ){
			THROW_IF( !j.is_number(), "Expected number '{}'.", j.dump() );
			T v = j;
			UA_Variant_setScalarCopy( &value, &v, value.type );
		}
		else
			throw Exception( "Arrays Not implemented." );
	}
}
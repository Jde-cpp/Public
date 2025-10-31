#pragma once
#include "opcHelpers.h"
#include "../exports.h"

namespace Jde::DB{ struct Row; struct Value; }
namespace Jde::Opc{
	struct ΓOPC NodeId : UA_NodeId{
		NodeId()ι:UA_NodeId{}{}
		NodeId( const NodeId& x )ι;
		NodeId( const UA_NodeId& x )ι;
		NodeId( NodeId&& x )ι;
		NodeId( UA_NodeId&& x )ι;
		NodeId( UA_UInt32 numeric )ι:NodeId{UA_NodeId{0, UA_NODEIDTYPE_NUMERIC, {numeric}}}{}
		NodeId( const flat_map<string,string>& x )ε;//rest params
		explicit NodeId( const jvalue& j )ε;
		NodeId( DB::Row& r, uint8 index )ε;

		α operator=( const NodeId& x )ι->NodeId&;
		α operator=( NodeId&& x )ι->NodeId&;
		α operator<( const NodeId& x )Ι->bool;

		Ω FromJson( const jvalue& v, UA_UInt16 ns=0 )ε->UA_NodeId;
		Ω FromJson( const jobject& j, UA_UInt16 ns=0 )ε->UA_NodeId;
		Ω IsSystem( const UA_NodeId& id )ι->bool;

		β InsertParams()Ι->vector<DB::Value>;
		α IsNumeric()Ι{ return identifierType==UA_NODEIDTYPE_NUMERIC; }
		α IsString()Ι{ return identifierType==UA_NODEIDTYPE_STRING; }
		α IsGuid()Ι{ return identifierType==UA_NODEIDTYPE_GUID; }
		α IsBytes()Ι{ return identifierType==UA_NODEIDTYPE_BYTESTRING; }
		α Numeric()Ι->optional<UA_UInt32>{ return identifierType==UA_NODEIDTYPE_NUMERIC ? identifier.numeric : optional<UA_UInt32>{}; }
		α String()Ι->optional<string>{ return identifierType==UA_NODEIDTYPE_STRING ? Opc::ToString(identifier.string) : optional<string>{}; }
		α Guid()Ι->optional<uuid>{ return identifierType==UA_NODEIDTYPE_GUID ? ToGuid(identifier.guid) : optional<uuid>{}; }
		α Bytes()Ι->optional<UA_ByteString>{ return identifierType==UA_NODEIDTYPE_BYTESTRING ? identifier.byteString : optional<UA_ByteString>{}; }

		α IsSystem()Ι->bool{ return IsNumeric() && IsSystem(*this); }
		α Add( jobject& j )Ι->void;
		α ToJson()Ι->jobject;
		α ToString()Ι->string;
	};
	α ToJson( const UA_NodeId& nodeId )ι->jobject;
}
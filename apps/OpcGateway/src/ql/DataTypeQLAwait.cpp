#include "DataTypeQLAwait.h"
#include "../async/ConnectAwait.h"
#include "../UAClient.h"

#define let const auto
namespace Jde::Opc::Gateway{
	α toJson( const UA_DataType& dt, const QL::TableQL& q )ι->jobject{
		jobject j;
		if( q.FindColumn("typeId") )
			j["typeId"] = NodeId{ dt.typeId }.ToJson();
		if( q.FindColumn("binaryEncodingId") )
			j["binaryEncodingId"] = NodeId{ dt.binaryEncodingId }.ToJson();
		if( q.FindColumn("xmlEncodingId") )
			j["xmlEncodingId"] = NodeId{ dt.xmlEncodingId }.ToJson();
		if( q.FindColumn("memSize") )
			j["memSize"] = dt.memSize;
		if( q.FindColumn("typeKind") )
			j["typeKind"] = dt.typeKind;
		if( q.FindColumn("pointerFree") )
			j["pointerFree"] = dt.pointerFree;
		if( q.FindColumn("overlayable") )
			j["overlayable"] = dt.overlayable;
		if( auto memberQL = q.FindTable("members"); memberQL ){
			jarray jMembers;
			for( uint m=0; m<dt.membersSize; ++m ){
				let member = &dt.members[m];
				jobject jMember;
				if( memberQL->FindColumn("padding") )
					jMember["padding"] = member->padding;
				if( memberQL->FindColumn("isArray") )
					jMember["isArray"] = ( bool )member->isArray;
				if( memberQL->FindColumn("isOptional") )
					jMember["isOptional"] = ( bool )member->isOptional;
				if( memberQL->FindColumn("type") )
					jMember["type"] = toJson( *member->memberType, *memberQL );
				jMembers.push_back( jMember );
			}
			j["members"] = jMembers;
		}
		return j;
	}
	//getRemoteDataTypes allocates the array with cleanup=true but does not add it to the client config, so the caller owns it. open62541's UA_cleanupDataTypeWithCustom is not exported; mirror it here.
	Ω freeRemoteDataTypes( UA_DataTypeArray* a )ι->void{
		while( a ){
			auto next = a->next;
			if( a->cleanup ){
				for( size_t i=0; i<a->typesSize; ++i ){
					const UA_DataType& t = a->types[i];
#ifdef UA_ENABLE_TYPEDESCRIPTION
					UA_free( (void*)(uintptr_t)t.typeName );
					for( size_t j=0; j<t.membersSize; ++j )
						UA_free( (void*)(uintptr_t)t.members[j].memberName );
#endif
					UA_free( (void*)t.members );
				}
				UA_free( (void*)(uintptr_t)a->types );
				UA_free( (void*)(uintptr_t)a );
			}
			a = next;
		}
	}
	α DataTypeQLAwait::Suspend()ι->void{
		try{
			jarray y;
			let nodeIds = NodeId::ParseQL( _ql );
			THROW_IF( nodeIds.empty(), "No nodeIds specified." );
			UA_DataTypeArray *customTypes;
			if( auto sc = UA_Client_getRemoteDataTypes(*_client, nodeIds.size(), nodeIds.data(), &customTypes); sc )
				throw UAClientException{ sc, _client->Handle(), Ƒ("Could not get data types: {}", NodeId::ToString(nodeIds)), _sl };
			for( uint i=0; i<customTypes->typesSize; ++i )
				y.push_back( toJson(customTypes->types[i], _ql) );
			freeRemoteDataTypes( customTypes );
			Resume( _ql.TransformResult(move(y)) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}
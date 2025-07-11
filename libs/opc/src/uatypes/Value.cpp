﻿#include <jde/opc/uatypes/Value.h>
#include <jde/opc/uatypes/UAClient.h>

#define let const auto

namespace Jde::Opc{
	namespace Read{
		Await::Await( flat_set<NodeId>&& x, sp<UAClient>&& c, SL sl )ι:IAwait{sl}, _nodes{move(x)}, _client{move(c)}{}

		α Await::Suspend()ι->void{
			_client->SendReadRequest( move(_nodes), _h );
		}
		//α Execute( sp<UAClient> ua, NodeId node, Web::Rest::Request req, bool snapShot )ι->Task;

		α OnResponse( UA_Client* ua, void* userdata, RequestId requestId, StatusCode sc, UA_DataValue* val )ι->void{
			let handle = userdata ? (RequestId)(uint)userdata : requestId;
			string logPrefix = format( "[{:x}.{}.{}]", (uint)ua, handle, requestId );
			if( sc )
				Trace( IotReadTag, "{}Value::OnResponse ({})-{} Value={}", logPrefix, sc, UAException::Message(sc), val ? serialize(Value{*val}.ToJson()) : "null" );
			auto pClient = UAClient::TryFind(ua); if( !pClient ) return;
			up<flat_map<NodeId, Value>> results;
			bool visited = pClient->_readRequests.visit( handle, [requestId, sc, val, &results, &logPrefix]( auto& pair ){
				auto& x = pair.second;
				if( auto pRequest=x.Requests.find(requestId); pRequest!=x.Requests.end() ){
					auto p = x.Results.try_emplace( pRequest->second, sc ? Value{ sc } : Value{ *val } ).first;
					Trace( IotReadTag, "{} Value={}", logPrefix, sc ? format("[{:x}]{}", sc, UAException::Message(sc)) : serialize(p->second.ToJson()) );
					if( x.Results.size()==x.Requests.size() )
						results = mu<flat_map<NodeId, Value>>( move(x.Results) );
				}
			});
			if( !visited )
				Critical( IotReadTag,  "{}Could not find handle.", logPrefix );
			else if( results ){
				pClient->_readRequests.erase( handle );
				auto h = pClient->ClearRequestH( handle ); if( !h ){ Critical{ IotReadTag, "[{}]Could not find handle.", logPrefix}; return; };
				Trace( IotReadTag, "{}Resume", logPrefix );
				Resume( move(results), h );
			}
			//DBG( "[{}]Value::~OnResponse()", logPrefix );
		}
	}
//#define ADD add.operator()
#define IS(ua) type==&UA_TYPES[ua]
	α Value::ToJson()Ι->jvalue{
		if( status )
			return jobject{ {"sc", status} };
		let scaler = IsScaler();
		let type = value.type;
		jvalue j{ scaler ? jvalue{} : jarray{} };
		auto add = [scaler, &j]( let& v )ι{
			if( scaler )
				j = v;
			else
				j.get_array().push_back( v );
		};
		auto addExplicit = [scaler, &j]( jvalue&& x ){ if( scaler ) j = x; else j.get_array().push_back(x); };
		for( uint i=0; i<(scaler ? 1 : value.arrayLength); ++i ){
			if( IS(UA_TYPES_BOOLEAN) )
				add( Get<UA_Boolean>(i) );
			else if( IS(UA_TYPES_BYTE) )
				addExplicit( (unsigned char)((UA_Byte*)value.data)[i] );
			else if( IS(UA_TYPES_BYTESTRING) ) [[unlikely]]
				addExplicit( ByteStringToJson(((UA_ByteString*)value.data)[i]) );
			else if( IS(UA_TYPES_DATETIME) )
				addExplicit( UADateTime{((UA_DateTime*)value.data)[i]}.ToJson() );
			else if( IS(UA_TYPES_DOUBLE) )
				add( Get<UA_Double>(i) );
			else if( IS(UA_TYPES_DURATION) ) [[unlikely]]
				add( Get<UA_Duration>(i) );
			else if( IS(UA_TYPES_EXPANDEDNODEID) ) [[unlikely]]
				addExplicit( Opc::ToJson( ((UA_ExpandedNodeId*)value.data)[i] ) );
			else if( IS(UA_TYPES_FLOAT) )
				add( Get<UA_Float>(i) );
			else if( IS(UA_TYPES_GUID) ) [[unlikely]]
				addExplicit( Opc::ToJson(((UA_Guid*)value.data)[i]) );
			else if( IS(UA_TYPES_INT16) ) [[likely]]
				add( Get<UA_Int16>(i) );
			else if( IS(UA_TYPES_INT32) ) [[likely]]
				add( Get<UA_Int32>(i) );
			else if( IS(UA_TYPES_INT64) )
				addExplicit( Opc::ToJson(((UA_Int64*)value.data)[i]) );
			else if( IS(UA_TYPES_NODEID) )
				addExplicit( Opc::ToJson((((UA_NodeId*)value.data)[i])) );
			else if( IS(UA_TYPES_SBYTE) )
				addExplicit( (char)((UA_SByte*)value.data)[i] );
			else if( IS(UA_TYPES_STATUSCODE) )
				add( Get<StatusCode>(i) );
			else if( IS(UA_TYPES_STRING) ) [[likely]]
				addExplicit( jstring{ToSV(((UA_String*)value.data)[i])} );
			else if( IS(UA_TYPES_UINT16) )
				add( Get<UA_UInt16>(i) );
			else if( IS(UA_TYPES_UINT32) ) [[likely]]
				add( Get<UA_UInt32>(i) );
			else if( IS(UA_TYPES_UINT64) )
				addExplicit( Opc::ToJson(((UA_UInt64*)value.data)[i]) );
			else if( IS(UA_TYPES_XMLELEMENT) ) [[unlikely]]
				addExplicit( jstring{ToSV(((UA_XmlElement*)value.data)[i])} );
			else{
				Warning( IotReadTag, "Unsupported type {}.", type->typeName );
				addExplicit( jstring{Ƒ("Unsupported type {}.", type->typeName)} );
			}
		}
		return j;
	}

	α Value::Set( const jvalue& j )ε->void{
		let scaler = UA_Variant_isScalar( &value );
		let type = value.type;
		if( IS(UA_TYPES_BOOLEAN) ){
			if( scaler ){
				THROW_IF( !j.is_bool(), "Expected bool '{}'.", serialize(j) );
				UA_Boolean v = j.get_bool();
				UA_Variant_setScalarCopy( &value, &v, type );
			}
			else
				throw Exception( "Not implemented." );
		}
		else if( IS(UA_TYPES_BYTE) )
			SetNumber<UA_Byte>( j );
		else if( IS(UA_TYPES_DOUBLE) )
			SetNumber<UA_Double>( j );
		else if( IS(UA_TYPES_FLOAT) )
			SetNumber<UA_Float>( j );
		else if( IS(UA_TYPES_INT16) )
			SetNumber<UA_Int16>( j );
		else if( IS(UA_TYPES_INT32) )
			SetNumber<UA_Int32>( j );
		else if( IS(UA_TYPES_INT64) )
			SetNumber<UA_Int64>( j );
		else if( IS(UA_TYPES_STRING) ){
			if( scaler ){
				THROW_IF( !j.is_string(), "Expected string '{}'.", serialize(j) );
				UA_String v = UA_String_fromChars( j.get_string().c_str() );
				UA_Variant_setScalarCopy( &value, &v, type );
			}
		}
		else if( IS(UA_TYPES_UINT16) )
			SetNumber<UA_UInt16>( j );
		else if( IS(UA_TYPES_UINT32) )
			SetNumber<UA_UInt32>( j );
		else if( IS(UA_TYPES_UINT64) )
			SetNumber<UA_UInt64>( j );
		else
			THROW( "Setting type '{}' has not been implemented.", type->typeName );
	}

	α Value::ToProto( const OpcClientNK& opcId, const NodeId& node )Ι->FromServer::Message{
		let scaler = IsScaler();
		let type = value.type;
		auto nv = mu<FromServer::NodeValues>(); nv->set_allocated_node( new Proto::ExpandedNodeId{node.ToProto()} ); nv->set_opc_id( opcId );
		//auto p = m.mutable_data_change();
		for( uint i=0; i<(scaler ? 1 : value.arrayLength); ++i ){
			auto& v = *nv->add_values();
			if( IS(UA_TYPES_BOOLEAN) )
				v.set_boolean( Get<UA_Boolean>(i) );
			else if( IS(UA_TYPES_BYTE) )
				v.set_byte( (uint8_t)Get<UA_Byte>(i) );
			else if( IS(UA_TYPES_BYTESTRING) ) [[unlikely]]
				v.set_allocated_byte_string( new string(ToSV(((UA_ByteString*)value.data)[i])) );
			else if( IS(UA_TYPES_DATETIME) )
				v.set_allocated_date( new google::protobuf::Timestamp{Get<UADateTime>(i).ToProto()} );
			else if( IS(UA_TYPES_DOUBLE) )
				v.set_double_value( Get<UA_Double>(i) );
			else if( IS(UA_TYPES_DURATION) ) [[unlikely]]
				v.set_allocated_duration( new google::protobuf::Duration{Get<UADateTime>(i).ToDuration()} );
			else if( IS(UA_TYPES_EXPANDEDNODEID) ) [[unlikely]]
				v.set_allocated_expanded_node( new Proto::ExpandedNodeId{NodeId{Get<UA_ExpandedNodeId>(i)}.ToProto()} );
			else if( IS(UA_TYPES_FLOAT) )
				v.set_float_value( Get<UA_Float>(i) );
			else if( IS(UA_TYPES_GUID) ) [[unlikely]]
				v.set_allocated_guid( new string{ToBinaryString(Get<UA_Guid>(i))} );
			else if( IS(UA_TYPES_INT16) ) [[likely]]
				v.set_int16( Get<UA_Int16>(i) );
			else if( IS(UA_TYPES_INT32) ) [[likely]]
				v.set_int32( Get<UA_Int32>(i) );
			else if( IS(UA_TYPES_INT64) )
				v.set_int64( Get<UA_Int64>(i) );
			else if( IS(UA_TYPES_NODEID) )
				v.set_allocated_node( new Proto::NodeId{NodeId{Get<UA_NodeId>(i)}.ToNodeProto()} );
			else if( IS(UA_TYPES_SBYTE) )
				v.set_sbyte( (int8_t)Get<UA_SByte>(i) );
			else if( IS(UA_TYPES_STATUSCODE) )
				v.set_status_code( Get<StatusCode>(i) );
			else if( IS(UA_TYPES_STRING) ) [[likely]]
				v.set_allocated_string_value( new string{ToSV(Get<UA_String>(i))} );
			else if( IS(UA_TYPES_UINT16) )
				v.set_uint16( Get<UA_UInt16>(i) );
			else if( IS(UA_TYPES_UINT32) ) [[likely]]
				v.set_uint32( Get<UA_UInt32>(i) );
			else if( IS(UA_TYPES_UINT64) )
				v.set_uint64( Get<UA_UInt64>(i) );
			else if( IS(UA_TYPES_XMLELEMENT) ) [[unlikely]]
				v.set_allocated_xml_element( new string{ToSV(Get<UA_XmlElement>(i))} );
			else{
				Warning( IotReadTag, "Unsupported type {}.", type->typeName );
				v.set_status_code( UA_STATUSCODE_BADNOTIMPLEMENTED );
			}
		}
		FromServer::Message m;
		m.set_allocated_node_values( nv.release() );
		return m;
	}
}
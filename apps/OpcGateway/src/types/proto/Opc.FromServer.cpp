#include "Opc.FromServer.h"
#include <jde/opc/uatypes/NodeId.h>
#include <jde/opc/uatypes/Value.h>

#define let const auto

namespace Jde::Opc::Gateway{
	α FromServer::AckTrans( uint32 socketSessionId )ι->FromServer::Transmission{
		FromServer::Message m;
		m.set_ack( socketSessionId );
		return MessageTrans( move(m), 0 );
	}

	α FromServer::CompleteTrans( RequestId requestId )ι->FromServer::Transmission{
		FromServer::Message m;
		return MessageTrans( move(m), requestId );
	}
	α FromServer::ExceptionTrans( const exception& e, optional<RequestId> requestId )ι->FromServer::Transmission{
		FromServer::Transmission t;
		auto& m = *t.add_messages();
		if( requestId )
			m.set_request_id( *requestId );

		auto& proto = *m.mutable_exception();
		proto.set_what( string{e.what()} );
		if( auto p = dynamic_cast<const IException*>(&e) )
			proto.set_code( p->Code );
		return t;
	}
	α FromServer::MessageTrans( FromServer::Message&& m, RequestId requestId )ι->FromServer::Transmission{
		FromServer::Transmission t;
		m.set_request_id( requestId );
		*t.add_messages() = move(m);
		return t;
	}
	α FromServer::SubscribeAckTrans( FromServer::SubscriptionAck&& ack, RequestId requestId )ι->FromServer::Transmission{
		FromServer::Transmission t;
		auto& m = *t.add_messages();
		m.set_request_id( requestId );
		m.set_allocated_subscription_ack( new FromServer::SubscriptionAck{move(ack)} );
		return t;
	}
	α FromServer::UnsubscribeTrans( uint32 id, flat_set<NodeId>&& successes, flat_set<NodeId>&& failures )ι->FromServer::Transmission{
		FromServer::Transmission t;
		auto& m = *t.add_messages();
		m.set_request_id( id );
		auto ack = m.mutable_unsubscribe_ack();
		for_each( move(successes), [&ack](let& n){*ack->add_successes() = ToNodeProto(n); } );
		for_each( move(failures), [&ack](let& n){*ack->add_failures() = ToNodeProto(n); } );
		return t;
	}

	α FromServer::ToProto( const ExNodeId& id )ι->Proto::ExpandedNodeId{
		Proto::ExpandedNodeId y;
		if( id.namespaceUri.length )
			y.set_allocated_namespace_uri( new string{ToSV(id.namespaceUri)} );
		y.set_server_index( id.serverIndex );
		y.set_allocated_node( new Proto::NodeId{ToNodeProto(id.nodeId)} );
		return y;
	}
#define IS(ua) type==&UA_TYPES[ua]
	α FromServer::ToProto( const ServerCnnctnNK& opcId, const NodeId& node, const Opc::Value& v )ι->FromServer::Message{
		let scaler = v.IsScaler();
		let type = v.value.type;
		auto nv = mu<FromServer::NodeValues>(); nv->set_allocated_node( new Proto::NodeId{ToNodeProto(node)} ); nv->set_opc_id( opcId );
		//auto p = m.mutable_data_change();
		for( uint i=0; i<(scaler ? 1 : v.value.arrayLength); ++i ){
			auto& proto = *nv->add_values();
			if( IS(UA_TYPES_BOOLEAN) )
				proto.set_boolean( v.Get<UA_Boolean>(i) );
			else if( IS(UA_TYPES_BYTE) )
				proto.set_byte( (uint8_t)v.Get<UA_Byte>(i) );
			else if( IS(UA_TYPES_BYTESTRING) ) [[unlikely]]
				proto.set_allocated_byte_string( new string(ToSV(((UA_ByteString*)v.value.data)[i])) );
			else if( IS(UA_TYPES_DATETIME) )
				proto.set_allocated_date( new google::protobuf::Timestamp{v.Get<UADateTime>(i).ToProto()} );
			else if( IS(UA_TYPES_DOUBLE) )
				proto.set_double_value( v.Get<UA_Double>(i) );
			else if( IS(UA_TYPES_DURATION) ) [[unlikely]]
				proto.set_allocated_duration( new google::protobuf::Duration{v.Get<UADateTime>(i).ToDuration()} );
			else if( IS(UA_TYPES_EXPANDEDNODEID) ) [[unlikely]]
				proto.set_allocated_expanded_node( new Proto::ExpandedNodeId{ToProto(v.Get<UA_ExpandedNodeId>(i))} );
			else if( IS(UA_TYPES_FLOAT) )
				proto.set_float_value( v.Get<UA_Float>(i) );
			else if( IS(UA_TYPES_GUID) ) [[unlikely]]
				proto.set_allocated_guid( new string{ToBinaryString(v.Get<UA_Guid>(i))} );
			else if( IS(UA_TYPES_INT16) ) [[likely]]
				proto.set_int16( v.Get<UA_Int16>(i) );
			else if( IS(UA_TYPES_INT32) ) [[likely]]
				proto.set_int32( v.Get<UA_Int32>(i) );
			else if( IS(UA_TYPES_INT64) )
				proto.set_int64( v.Get<UA_Int64>(i) );
			else if( IS(UA_TYPES_NODEID) )
				proto.set_allocated_node( new Proto::NodeId{ ToNodeProto(v.Get<UA_NodeId>(i))} );
			else if( IS(UA_TYPES_SBYTE) )
				proto.set_sbyte( (int8_t)v.Get<UA_SByte>(i) );
			else if( IS(UA_TYPES_STATUSCODE) )
				proto.set_status_code( v.Get<StatusCode>(i) );
			else if( IS(UA_TYPES_STRING) ) [[likely]]
				proto.set_allocated_string_value( new string{ToSV(v.Get<UA_String>(i))} );
			else if( IS(UA_TYPES_UINT16) )
				proto.set_uint16( v.Get<UA_UInt16>(i) );
			else if( IS(UA_TYPES_UINT32) ) [[likely]]
				proto.set_uint32( v.Get<UA_UInt32>(i) );
			else if( IS(UA_TYPES_UINT64) )
				proto.set_uint64( v.Get<UA_UInt64>(i) );
			else if( IS(UA_TYPES_XMLELEMENT) ) [[unlikely]]
				proto.set_allocated_xml_element( new string{ToSV(v.Get<UA_XmlElement>(i))} );
			else{
				WARNT( IotReadTag, "Unsupported type {}.", type->typeName );
				proto.set_status_code( UA_STATUSCODE_BADNOTIMPLEMENTED );
			}
		}
		FromServer::Message m;
		m.set_allocated_node_values( nv.release() );
		return m;
	}
	α FromServer::ToNodeProto( const NodeId& id )ι->Proto::NodeId{
		Proto::NodeId y;
		y.set_namespace_index( id.namespaceIndex );
		if( id.identifierType==UA_NodeIdType::UA_NODEIDTYPE_NUMERIC )
			y.set_numeric( id.identifier.numeric );
		else if( id.identifierType==UA_NodeIdType::UA_NODEIDTYPE_STRING )
			y.set_allocated_string( new string{ToSV(id.identifier.string)} );
		else if( id.identifierType==UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING )
			y.set_allocated_byte_string( new string{ToSV(id.identifier.byteString)} );
		else if( id.identifierType==UA_NodeIdType::UA_NODEIDTYPE_GUID )
			y.set_guid( ToBinaryString(id.identifier.guid) );
		return y;
	}
}
#pragma once
namespace Jde::Opc{ struct ExNodeId; struct Value; }

namespace Jde::Opc::Gateway::FromServer{
	α AckTrans( uint32 socketSessionId )ι->FromServer::Transmission;
	α CompleteTrans( RequestId requestId )ι->FromServer::Transmission;
	α SubscribeAckTrans( FromServer::SubscriptionAck&& ack, RequestId requestId )ι->FromServer::Transmission;
	α UnsubscribeTrans( uint32 id, flat_set<ExNodeId>&& successes, flat_set<ExNodeId>&& failures )ι->FromServer::Transmission;
	α MessageTrans( FromServer::Message&& m, RequestId requestId )ι->FromServer::Transmission;
	α ExceptionTrans( const exception& e, optional<RequestId> requestId )ι->FromServer::Transmission;

	α ToProto( const OpcClientNK& opcId, const ExNodeId& node, const Opc::Value& v )ι->FromServer::Message;
	α ToProto( const ExNodeId& id )ι->Proto::ExpandedNodeId;
	α ToNodeProto( const ExNodeId& id )ι->Proto::NodeId;
}
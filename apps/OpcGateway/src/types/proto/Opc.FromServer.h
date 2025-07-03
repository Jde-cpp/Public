#pragma once
namespace Jde::Opc{ struct NodeId; }

namespace Jde::Opc::FromServer{
	α AckTrans( uint32 socketSessionId )ι->FromServer::Transmission;
	α CompleteTrans( RequestId requestId )ι->FromServer::Transmission;
	α SubscribeAckTrans( up<FromServer::SubscriptionAck>&& ack, RequestId requestId )ι->FromServer::Transmission;
	α UnsubscribeTrans( uint32 id, flat_set<NodeId>&& successes, flat_set<NodeId>&& failures )ι->FromServer::Transmission;
	α MessageTrans( FromServer::Message&& m, RequestId requestId )ι->FromServer::Transmission;
	α ExceptionTrans( const exception& e, optional<RequestId> requestId )ι->FromServer::Transmission;
}
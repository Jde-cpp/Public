#pragma once
#include "Opc.Common.pb.h"
#include "../proto/Opc.FromClient.pb.h"
namespace Jde::Opc{ struct NodeId; struct Value; }

namespace Jde::Opc::Gateway::FromClientUtils{
	α Connection( RequestId requestId, SessionPK sessionId )ι->string;
	α Subscription( RequestId requestId, ServerCnnctnNK&& target, const vector<NodeId>& nodes )ι->string;
	α Unsubscription( RequestId requestId, ServerCnnctnNK&& target, const vector<NodeId>& nodes )ι->string;
	α Query( RequestId requestId, string&& query, jobject&& variables, bool returnRaw )ι->string;
}

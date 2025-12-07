#pragma once
#include <boost/beast/ssl.hpp>
#include <jde/web/usings.h>
#include <jde/web/client/socket/IClientSocketSession.h>
#include <jde/web/client/socket/ClientSocketAwait.h>
#include <jde/opc/uatypes/NodeId.h>
#include "../../src/types/proto/Opc.FromClient.pb.h"
#include "../../src/types/proto/Opc.FromServer.pb.h"
#include <jde/app/proto/Common.pb.h>

namespace Jde::Opc::Gateway::Tests{
	using namespace Jde::Web::Client;
	struct IListener{
		β OnData( string opcId, NodeId nodeId, const vector<FromServer::Value>& values )ι->void=0;
	};
	struct GatewayClientSocket final : TClientSocketSession<FromClient::Transmission,FromServer::Transmission>{
		using base = TClientSocketSession<FromClient::Transmission,FromServer::Transmission>;
		Τ using await = Web::Client::ClientSocketAwait<T>;
		GatewayClientSocket( sp<net::io_context> ioc, optional<ssl::context>& ctx )ι;
		~GatewayClientSocket(){ TRACET(ELogTags::Test, "GatewayClientSocket::~GatewayClientSocket"); }

		α Connect( SessionPK sessionId, SRCE )ι->ClientSocketAwait<uint32>;
		α Query( string&& query, jobject variables, bool returnRaw, SRCE )ι->ClientSocketAwait<jvalue> override;
		α Subscribe( ServerCnnctnNK target, const vector<NodeId>& nodes, sp<IListener> listener, SRCE )ε->await<FromServer::SubscriptionAck>;
		α Subscribe( string&& /*query*/, jobject /*variables*/, sp<QL::IListener> /*listener*/, SL )ε->await<jarray>{ ASSERT(false); throw "noimpl"; }
		α LogSubscribe( jobject&& ql, jobject vars, sp<IListener> listener, SRCE )ε->ClientSocketAwait<jarray>;

		α Unsubscribe( ServerCnnctnNK target, const vector<NodeId>& nodeIds, SRCE )ε->ClientSocketAwait<FromServer::UnsubscribeAck>;
	private:
		α HandleException( std::any&& h, Jde::Proto::Exception&& what )ι;
		α OnRead( FromServer::Transmission&& transmission )ι->void override;
		α OnClose( beast::error_code ec )ι->void override;
		α OnAck( uint32 ack )ι->void;
	};
}
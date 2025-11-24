#pragma once
#include <jde/web/server/Sessions.h>
#include <jde/web/client/usings.h>
#include "types/MonitoringNodes.h"
#include <jde/web/server/IWebsocketSession.h>

namespace Jde::Proto{ struct Query; }
namespace Jde::Opc{ struct NodeId; }
namespace Jde::Opc::Gateway{
	using namespace Jde::Web::Server;
	using namespace Jde::Web::Client;
	struct GatewaySocketSession : TWebsocketSession<FromServer::Transmission,FromClient::Transmission>, IDataChange{
		using base = TWebsocketSession<FromServer::Transmission,FromClient::Transmission>;
		GatewaySocketSession( sp<RestStream> stream, beast::flat_buffer&& buffer, TRequestType&& request, tcp::endpoint&& userEndpoint, uint32 connectionIndex )ι;
		α OnRead( FromClient::Transmission&& transmission )ι->void override;
		α SendDataChange( const ServerCnnctnNK& opcNK, const NodeId& node, const Value& value )ι->void override;
		α to_string()Ι->string override{ return Ƒ( "{:x}", Id() ); }
		α WriteException( exception&& e, Jde::RequestId requestId )ι->void override;
	private:
		α CreateSubscription( sp<UAClient> client, flat_set<NodeId> nodes, RequestId requestId )ι->VoidAwait::Task;
		α OnClose()ι->void;
		α ProcessTransmission( FromClient::Transmission&& transmission )ι->void;
		α SetSessionId( str sessionId, RequestId requestId )->Sessions::UpsertAwait::Task;
		α Schemas()Ι->const vector<sp<DB::AppSchema>>&;
		α SharedFromThis()ι->sp<GatewaySocketSession>{ return std::dynamic_pointer_cast<GatewaySocketSession>(shared_from_this()); }

		α GraphQL( Jde::Proto::Query&& q, uint requestId )ι->TAwait<jvalue>::Task;
		α Subscribe( ServerCnnctnNK&& opcId, flat_set<NodeId> nodes, uint32 requestId )ι->TAwait<sp<UAClient>>::Task;
		α Unsubscribe( ServerCnnctnNK&& opcId, flat_set<NodeId> nodes, uint32 requestId )ι->void;

		α WriteSubscription( const jvalue& j, Jde::RequestId requestId )ι->void override;
		α WriteSubscriptionAck( vector<QL::SubscriptionId>&& subscriptionIds, Jde::RequestId requestId )ι->void override;
		α WriteComplete( Jde::RequestId requestId )ι->void override;
		α WriteException( string&& e, Jde::RequestId requestId )ι->void override;
		α WriteException( IException&& e )ι->void{ WriteException( move(e), 0 ); }

		α SendAck( uint32 id )ι->void override;
	};
}
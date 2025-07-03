#pragma once
#include <jde/web/server/Sessions.h>
#include <jde/web/client/usings.h>
#include <jde/opc/types/MonitoringNodes.h>
#include <jde/web/server/IWebsocketSession.h>

namespace Jde::Opc{
	using namespace Jde::Web::Server;
	using namespace Jde::Web::Client;
	struct NodeId;
	struct ServerSocketSession : TWebsocketSession<FromServer::Transmission,FromClient::Transmission>, IDataChange{
		using base = TWebsocketSession<FromServer::Transmission,FromClient::Transmission>;
		ServerSocketSession( sp<RestStream> stream, beast::flat_buffer&& buffer, TRequestType&& request, tcp::endpoint&& userEndpoint, uint32 connectionIndex )ι;
		α OnRead( FromClient::Transmission&& transmission )ι->void override;
		α SendDataChange( const Jde::Opc::OpcClientNK& opcNK, const Jde::Opc::NodeId& node, const Jde::Opc::Value& value )ι->void override;
		α to_string()Ι->string override{ return Ƒ( "{:x}", Id() ); }

	private:
		α CreateSubscription( sp<UAClient> client, flat_set<NodeId> nodes, uint32 requestId )ι->Task;
		α OnClose()ι->void;
		α ProcessTransmission( FromClient::Transmission&& transmission )ι->void;
		α SetSessionId( str sessionId, RequestId requestId )->Sessions::UpsertAwait::Task;
		α SharedFromThis()ι->sp<ServerSocketSession>{ return std::dynamic_pointer_cast<ServerSocketSession>(shared_from_this()); }
		α Subscribe( OpcClientNK&& opcId, flat_set<NodeId> nodes, uint32 requestId )ι->void;
		α Unsubscribe( OpcClientNK&& opcId, flat_set<NodeId> nodes, uint32 requestId )ι->void;

		α WriteSubscription( const jvalue& j, Jde::RequestId requestId )ι->void override;
		α WriteSubscriptionAck( vector<QL::SubscriptionId>&& subscriptionIds, Jde::RequestId requestId )ι->void override;
		α WriteComplete( Jde::RequestId requestId )ι->void override;
		α WriteException( string&& e, Jde::RequestId requestId )ι->void override;
		α WriteException( exception&& e, Jde::RequestId requestId )ι->void override;
		α WriteException( IException&& e )ι->void{ WriteException( move(e), 0 ); }

		α GraphQL( string&& query, uint requestId )ι->Task;
		α SendAck( uint32 id )ι->void override;
	};
}
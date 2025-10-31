#pragma once
#include <jde/ql/QLHook.h>
#include "../uatypes/Browse.h"
#include "../async/ReadAwait.h"

namespace Jde::Web::Server{ struct SessionInfo; }
namespace Jde::QL{ struct TableQL; }
namespace Jde::Opc::Gateway{
	struct UAClient;
	struct NodeQLAwait final: TAwaitEx<jvalue,TAwait<sp<UAClient>>::Task>{
		using base = TAwaitEx<jvalue,TAwait<sp<UAClient>>::Task>;
		NodeQLAwait( QL::TableQL&& query, SessionPK sessionPK, UserPK executer, SRCE )ι:
			base{ sl }, _executer{executer}, _query{move(query)}, _sessionPK{move(sessionPK)}
		{}
		α Execute()ι->TAwait<sp<UAClient>>::Task override;
	private:
		using ExpectedNodeId = std::expected<ExNodeId,StatusCode>;
		using BrowsePathResponse = flat_map<string,ExpectedNodeId>;
		α AddAttributes( ExpectedNodeId nodeId, QL::TableQL* parentsQL, flat_map<NodeId, jobject> parents )ι->TAwait<ReadResponse>::Task;
		α AddAttributes( Browse::Response&& br, QL::TableQL&& childrenQL, flat_map<NodeId, jobject> jChildren )ι->TAwait<ReadResponse>::Task;
		α Browse( BrowsePathResponse pathNodes, str lastGoodParent, QL::TableQL* parents, flat_map<NodeId, jobject> jParents )ι->TAwait<Browse::Response>::Task;
		α Browse( NodeId parentId, QL::TableQL children )ι->TAwait<Browse::Response>::Task;

		sp<UAClient> _client;
		UserPK _executer;
		QL::TableQL _query;
		SessionPK _sessionPK;
	};
}
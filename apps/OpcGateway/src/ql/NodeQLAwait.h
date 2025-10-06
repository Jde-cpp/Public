#pragma once
#include <jde/ql/QLHook.h>

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
		α AddAttributes( flat_map<string, std::expected<ExNodeId,StatusCode>>&& pathNodes, QL::TableQL* parents, string reqPath )ι->void;

		sp<UAClient> _client;
		UserPK _executer;
		QL::TableQL _query;
		SessionPK _sessionPK;
	};
}
#pragma once
#include <jde/web/server/IHttpRequestAwait.h>
#include <jde/opc/uatypes/Value.h>

namespace Jde::Opc{ struct NodeId; }
namespace Jde::Opc::Gateway{
	struct UAClient;
	using namespace Jde::Web::Server;
	struct HttpRequestAwait final: IHttpRequestAwait{
		using base = IHttpRequestAwait;
		HttpRequestAwait( HttpRequest&& req, SRCE )ι;
		α await_ready()ι->bool override;
		α Suspend()ι->void override;
		α await_resume()ε->HttpTaskResult override;
	private:
		α Login( str endpoint )ι->TAwait<optional<Web::FromServer::SessionInfo>>::Task;
		α Logout()ι->TAwait<jvalue>::Task;
		α CoHandleRequest( ServerCnnctnNK&& opcId )ι->TAwait<sp<UAClient>>::Task;
		α Browse()ι->TAwait<jobject>::Task;
		α Query()ι->TAwait<HttpTaskResult>::Task;
		α ParseNodes()ε->tuple<flat_set<NodeId>,jarray>;
		α ResumeSnapshots( flat_map<NodeId, Value>&& results, jarray&& j )ι->void;
		α SnapshotWrite( flat_set<NodeId>&& nodes, flat_map<NodeId, Value>&& values, jarray&& jNodes )ι->TAwait<flat_map<NodeId,UA_WriteResponse>>::Task;
		α SnapshotRead( bool write={} )ι->TAwait<flat_map<NodeId, Value>>::Task;
		sp<UAClient> _client;
	};
}
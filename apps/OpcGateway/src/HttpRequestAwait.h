#pragma once
#include <jde/web/server/IHttpRequestAwait.h>
#include <jde/web/server/Sessions.h>
#include <jde/opc/UM.h>
#include <jde/opc/uatypes/Browse.h>
#include <jde/opc/async/ConnectAwait.h>

namespace Jde::Opc{
	struct NodeId; struct Value;
	using namespace Jde::Web::Server;
	struct HttpRequestAwait final: IHttpRequestAwait{
		using base = IHttpRequestAwait;
		HttpRequestAwait( HttpRequest&& req, SRCE )ι;
		α await_ready()ι->bool override;
		α Suspend()ι->void override;
		α await_resume()ε->HttpTaskResult override;
	private:
		α Login( str endpoint )ι->AuthenticateAwait::Task;
		α Logout()ι->TAwait<jvalue>::Task;
		α CoHandleRequest()ι->ConnectAwait::Task;
		α Browse()ι->Browse::ObjectsFolderAwait::Task;
		α ParseNodes()ε->tuple<flat_set<NodeId>,jarray>;
		α ResumeSnapshots( flat_map<NodeId, Value>&& results, jarray&& j )ι->void;
		α SnapshotWrite()ι->Jde::Task;
		sp<UAClient> _client;
	};
}
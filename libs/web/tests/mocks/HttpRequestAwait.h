#pragma once
#include <jde/web/server/IHttpRequestAwait.h>

namespace Jde::Web::Mock{
	using namespace Jde::Web::Server;
	struct HttpRequestAwait final: IHttpRequestAwait{
		using base = IHttpRequestAwait;
		HttpRequestAwait( HttpRequest&& req, SRCE )ι;
		α await_ready()ι->bool override;
		α Suspend()ι->void override;
		α await_resume()ε->HttpTaskResult override;
		α QueryHandler( QL::RequestQL&&, variant<sp<SessionInfo>, Jde::UserPK>, bool, SL )ι->up<IQLAwait>{ASSERT(false);return {};}
		α Schemas()Ι->const vector<sp<DB::AppSchema>>&{ ASSERT(false); return _schemas;}
	private:
		optional<HttpTaskResult> _result;
		optional<std::jthread> _thread;
		vector<sp<DB::AppSchema>> _schemas;//always empty
	};
}
#pragma once
#include "../../../../../Framework/source/coroutine/Awaitable.h"
#include "ClientHttpRes.h"
#include <jde/coroutine/Await.h>
#include "../usings.h"

namespace Jde::Web::Client{
	struct ClientHttpSession;
	α RemoveHttpSession( sp<ClientHttpSession> pSession )ι;

	struct HttpAwaitArgs {
		string Authorization;
		string ContentType{ "application/x-www-form-urlencoded" };
		optional<boost::beast::http::verb> Verb{ http::verb::unknown };
		const bool IsSsl{ true };
	};
	struct ClientHttpAwait : TAwait<ClientHttpRes>, HttpAwaitArgs{
		using base = TAwait<ClientHttpRes>;
		ClientHttpAwait( string host, string target, string body, PortType port=443, HttpAwaitArgs args={}, SRCE )ι;
		ClientHttpAwait( string host, string target, PortType port=443, HttpAwaitArgs args={}, SRCE )ι;
		ClientHttpAwait( ClientHttpAwait&& from )ι;
		α await_suspend( base::Handle h )ε->void override{ base::await_suspend(h); Execute(); }
		α await_resume()ε->ClientHttpRes override;
	protected:
		α Execute()ι->TAwait<ClientHttpRes>::Task;
		string _host;
		string _target;
		string _body;
		PortType _port;
		sp<net::io_context> _ioContext;
	};

	struct ClientHttpAwaitSingle final : ClientHttpAwait{
		using base = ClientHttpAwait;
		ClientHttpAwaitSingle( ClientHttpAwait&& from )ι:base{ move(from) }{};
		α await_ready()ι->bool override{ return _ioContext==nullptr; }
		α await_suspend( base::Handle h )ε->void override{ TAwait<ClientHttpRes>::await_suspend(h); Execute(); }
		α await_resume()ε->ClientHttpRes override;
	private:
		α Execute()ι->void;
	};
}
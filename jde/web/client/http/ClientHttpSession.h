#pragma once
#include "../usings.h"
#include "ClientHttpStream.h"
#include "ClientHttpAwait.h"

namespace Jde::Web::Client{
	//struct HttpAwaitArgs;

	struct HttpRequestRunArgs {
		string ContentType{ "application/x-www-form-urlencoded" };
		optional<boost::beast::http::verb> _verb{ http::verb::unknown };
	};

	struct ΓWC ClientHttpSession : public std::enable_shared_from_this<ClientHttpSession>{
    ClientHttpSession( str host, PortType port, net::any_io_executor strand )ε;
		ClientHttpSession( str host, PortType port, net::any_io_executor strand, bool isPlain )ε;
		Ω Key( str host, PortType port, bool isSsl)ι->string{ return Ƒ("http{}//{}:{}", isSsl ? "s" : "", host, port); }

		α Close()ε->VoidTask;
		α IsRunning()Ι{ return _isRunning.test(); } α SetIsRunning( bool x )ι->void{ if( x ) _isRunning.test_and_set(); else _isRunning.clear(); }
		α Resolver()ι->tcp::resolver&{ return _resolver; }
		α Run()ε->VoidTask;
		α Send( string target, string body, const HttpAwaitArgs& args, ClientHttpAwaitSingle::Handle h )ι->VoidTask;
		α Stream()ι->ClientHttpStream&{ return _stream; }

		const string Host;
		const PortType Port;
		const bool IsSsl;
		const bool AllowRedirects{ true };
	private:
		α Write( string target, string body, const HttpAwaitArgs& args, ClientHttpAwaitSingle::Handle h )ι->TAwait<ClientHttpRes>::Task;
		bool _isConnected{ false }; //TODO!
		string _authorization;
    beast::flat_buffer _buffer;
		tcp::resolver _resolver;
		atomic_flag _isRunning;
    ClientHttpStream _stream;
    http::request<http::empty_body> _req;
    http::response<http::string_body> _res;
	};
}
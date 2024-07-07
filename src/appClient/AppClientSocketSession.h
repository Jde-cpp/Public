#pragma once
#include <jde/http/ClientSocketAwait.h>
#include <jde/http/IClientSocketSession.h>
#include "usings.h"
#include "App.FromClient.pb.h"
#include "App.FromServer.pb.h"

namespace Jde::App::Client{
	struct StartSocketAwait : VoidAwait<>{
		using base = VoidAwait<>;
		StartSocketAwait( str host, PortType port, bool isSsl, SRCE )ι:base{sl}, _host{host}, _port{port}, _isSsl{isSsl}{}
		α await_suspend( base::Handle h )ι->void override;
		string _host; PortType _port; bool _isSsl;
	};

	α CloseSocketSession()ι->VoidTask;
	struct AppClientSocketSession final : Http::TClientSocketSession<Jde::App::Proto::FromClient::Transmission,Jde::App::Proto::FromServer::Transmission>{
		using base = Http::TClientSocketSession<Proto::FromClient::Transmission,Proto::FromServer::Transmission>;
		Ω Instance()ι->sp<AppClientSocketSession>;
		AppClientSocketSession( sp<net::io_context> ioc, optional<ssl::context> ctx )ι;
		α Connect( SessionPK sessionId, SRCE )ι->Http::ClientSocketAwait<SessionPK>;
		α SessionInfo( SessionPK sessionId, SRCE )ι->Http::ClientSocketAwait<Proto::FromServer::SessionInfo>;

	private:
		α HandleException( std::any&& h, string&& what )ι->void;
		α OnRead( Proto::FromServer::Transmission&& transmission )ι->void override;
		α OnClose( beast::error_code ec )ι->void override;
	};
}
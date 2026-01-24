#pragma once
#include "usings.h"
#include <jde/fwk/crypto/CryptoSettings.h>
#include <jde/app/IApp.h>

namespace Jde::DB{ struct AppSchema; }
namespace Jde::Web::Server{
	struct HttpRequest; struct IHttpRequestAwait; struct IWebsocketSession;struct RestStream;
	struct IRequestHandler{
		IRequestHandler( jobject settings, sp<App::IApp> appServer )ι;
		virtual ~IRequestHandler()=default; //msvc error
		β HandleRequest( HttpRequest&& req, SRCE )ι->up<IHttpRequestAwait> =0;
		β PassQL()ι->bool{ return true; }
		β Query( QL::RequestQL&& ql, UserPK executer, bool raw, SRCE )ε->up<TAwait<jvalue>> = 0;
		β Schemas()ι->const vector<sp<DB::AppSchema>>& =0;
		β WebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType req, tcp::endpoint userEndpoint, uint32 connectionIndex )ι->sp<IWebsocketSession> =0;

		α AppServer()ι->sp<App::IApp>{ return _appServer; }
		α AppServerLocal()ι->bool{ return _appServer->IsLocal(); }
		α AppQueryAwait( string&& q, jobject variables, SL sl )ι->up<TAwait<jvalue>>{ return _appServer->Query<jvalue>( move(q), move(variables), true, sl ); }
		α CancelSignal()ι->sp<net::cancellation_signal>{ return _cancelSignal; }
		α Context()ι->ssl::context&{ return _ctx; }
		α NextRequestId()ι->uint32{ return _requestId.fetch_add(1, std::memory_order_relaxed); }
		α SessionInfoAwait( SessionPK sessionPK, SL sl )ι->up<TAwait<Web::FromServer::SessionInfo>>{ return _appServer->SessionInfoAwait( sessionPK, sl ); }
		α Start()ι->void;
		α Stop()ι->void;
		α BlockTillStarted()ι->void;

		struct WebServerSettings{
			WebServerSettings( jobject settings )ι:_crypto{Json::FindDefaultObject(settings, "ssl")}, _settings(move(settings)){}
			α Address()Ι->string{ return Json::FindString(_settings, "address" ).value_or( "0.0.0.0" ); }
			α Port()Ι->PortType{ return Json::FindNumber<PortType>(_settings, "port" ).value_or( 6809 ); }
			α DhPath()Ι->string{ return Json::FindString( _settings,  "dh" ).value_or( "/etc/ssl/certs/server.crt" ); }
			α CertPath()Ι->string{ return Json::FindString( _settings, "cert" ).value_or( "/etc/ssl/private/server.key" ); }
			α Crypto()Ι->const Crypto::CryptoSettings&{ return _crypto; }
		private:
			Crypto::CryptoSettings _crypto;
			jobject _settings;
		};
	α Settings()Ι->const WebServerSettings&{ return _settings; }
	private:
		sp<App::IApp> _appServer;
		sp<net::cancellation_signal> _cancelSignal;
		ssl::context _ctx;
		atomic<uint32> _requestId;
		WebServerSettings _settings;
		atomic_flag _started{};
	};
}
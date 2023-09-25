#pragma once
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include "TypeDefs.h"
#include <jde/coroutine/Task.h>

	//https://www.boost.org/doc/libs/1_73_0/libs/beast/example/http/server/async/http_server_async.cpp

namespace Jde::Web::Rest
{
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace net = boost::asio;
	using tcp = boost::asio::ip::tcp;
	struct IRequestException : Exception
	{
		IRequestException()ι:Exception{""}{}
		template<class... Args> IRequestException( SL sl, sv fmt, Args&&... args )ι:Exception( sl, ELogLevel::Debug, fmt, args... ){}
		template<class... Args> IRequestException( SL sl, std::exception&& inner, sv format_={}, Args&&... args )ι:Exception{sl, move(inner), format_, args...}{}
		β Status()Ι->http::status=0;
	private:
		string _clientMessage;
	};

	template<http::status TStatus>
	struct RequestException : IRequestException
	{
		RequestException()ι:IRequestException{}{}
		template<class... Args> RequestException( SL sl, sv fmt, Args&&... args )ι:IRequestException( sl, fmt, args... ){}
		template<class... Args> RequestException( SL sl, std::exception&& inner, sv format_={}, Args&&... args )ι:IRequestException{sl, move(inner), format_, args...}{}
		α Status()Ι->http::status{ return TStatus; }
	};

	struct SessionInfoAwait final : IAwaitCache
	{
		SessionInfoAwait( SessionPK sessionId, SRCE )ι:IAwaitCache{sl}, _sessionId{sessionId}{}
		α await_ready()ι->bool override;
		α await_suspend( HCoroutine h )ι->void override;
	private:
		AwaitResult _result;
		const SessionPK _sessionId;
	};

	struct ISession;
	struct Request final
	{
		//http::request<http::string_body> ClientRequest;
		sp<ISession> Session;
		up<SessionInfo> SessionInfoPtr;
		α ClientRequest()Ι->const http::request<http::string_body>&;
		α UserId()Ι->UserPK{ return SessionInfoPtr ? SessionInfoPtr->user_id() : 0; }
		α Method()Ι->http::verb{ return ClientRequest().method(); }
	};

	struct ISession
	{
		using TMessage = http::message<true, http::string_body, http::basic_fields<std::allocator<char>>>;
    ISession( tcp::socket&& socket )ι: _stream{move(socket)}//,_send{*this}
    {}

    α Run()ι->void{ net::dispatch( _stream.get_executor(), beast::bind_front_handler( &ISession::DoRead, MakeShared()) ); }
		α DoRead()ι->void;
    α OnRead( beast::error_code ec, size_t bytes_transferred )ι->void;
    //template<bool isRequest, class Body, class Fields>
		//α Send( http::message<isRequest, Body, Fields>&& msg )->void;

		α ClientRequest()Ι->const http::request<http::string_body>&{ return _request; }
		α OnWrite( bool close, beast::error_code ec, size_t bytes_transferred )ι->void;
		α DoClose()ι->void;
		β HandleRequest( string&& target, flat_map<string,string>&& params, Request&& req )ι->void=0;
		α HandleRequest( Request req )ι->Task;

		Ω Send( Exception&& e, Request&& req )ι->void;
		Ω Send( http::status status, string what, Request&& req )ι->void;
		Ω Send( const IRequestException&& e, Request&& req )ι->void;
		Ω Send( string&& value, Request&& req )ι->void;
		Ω Send( const json& payload, Request&& req )ι->void;
		Ω SendOptions( Request&& req )ι->void;

		β MakeShared()ι->sp<ISession> = 0;

		α SendQuery( string&& query, Request&& req )ι->Task;
		α AddSession( UserPK userId )ι->sp<SessionInfo>;
		Ω FetchSessionInfo( SessionPK sessionId )ι->SessionInfoAwait{ return SessionInfoAwait{ sessionId }; }
	private:
	  template<bool isRequest, class Body, class Fields>
		Ω Send( sp<http::message<isRequest, Body, Fields>>&& m, sp<ISession>&& s )->void;

    beast::tcp_stream _stream;
    beast::flat_buffer _buffer;
    http::request<http::string_body> _request;
    sp<void> _message;
	};

	struct Γ IListener
	{
	  IListener( PortType defaultPort )ε;
		virtual ~IListener(){}
    α Run()ι->void{ DoAccept(); }
	  α DoAccept()ι->void;
	private:
	  α OnAccept(beast::error_code ec, tcp::socket socket)ι->void;
		β CreateSession( tcp::socket&& socket )ι->sp<ISession> =0;
		virtual auto MakeShared()ι->sp<void> =0;

		sp<IO::AsioContextThread> _pIOContext;
		tcp::acceptor _acceptor;
	};

	Ξ Request::ClientRequest()Ι->const http::request<http::string_body>&{ ASSERT(Session); return Session->ClientRequest(); }
	template<class TSession>
	struct TListener : IListener, std::enable_shared_from_this<TListener<TSession>>
	{
	  TListener( PortType port )ε:IListener{ port }{}
		β CreateSession( tcp::socket&& socket )ι->sp<ISession> override;
		α MakeShared()ι->sp<void> override{ return this->shared_from_this(); }
	};

  template<bool isRequest, class Body, class Fields>
	α ISession::Send( sp<http::message<isRequest, Body, Fields>>&& m, sp<ISession>&& s )->void
	{
    s->_message = m;
    http::async_write( s->_stream, *m, beast::bind_front_handler(&ISession::OnWrite, move(s), m->need_eof()) );
	}

	template<class TSession>
  α TListener<TSession>::CreateSession( tcp::socket&& socket )ι->sp<ISession>
  {
		return  ms<TSession>( move(socket) );
  }

}
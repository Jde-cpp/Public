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
		β Status()->http::status=0;
	};

	template<http::status TStatus>
	struct RequestException : IRequestException
	{
		RequestException()ι:IRequestException{}{}
		template<class... Args> RequestException( SL sl, sv fmt, Args&&... args )ι:IRequestException( sl, fmt, args... ){}
		template<class... Args> RequestException( SL sl, std::exception&& inner, sv format_={}, Args&&... args ):IRequestException{sl, move(inner), format_, args...}{}
		α Status()->http::status{ return TStatus; }
	};

	struct SessionInfoAwait final : IAwait
	{
		SessionInfoAwait( SessionPK sessionId, SRCE )ι:IAwait{sl}, _sessionId{sessionId}{}
		α await_ready()ι->bool override;
		α await_suspend( HCoroutine h )ι->void override;
		α await_resume()ι->AwaitResult override{ return _pPromise ? _pPromise->get_return_object().Result() : move(_result); }
	private:
		AwaitResult _result;
		const SessionPK _sessionId;
	};

	struct ISession
	{
		using TMessage = http::message<true, http::string_body, http::basic_fields<std::allocator<char>>>;
    ISession( tcp::socket&& socket ): Stream{move(socket)}//,_send{*this}
    {}

    α Run()ι->void{ net::dispatch( Stream.get_executor(), beast::bind_front_handler( &ISession::DoRead, MakeShared()) ); }
		α DoRead()ι->void;
    α OnRead( beast::error_code ec, size_t bytes_transferred )ι->void;
    //template<bool isRequest, class Body, class Fields>
		//α Send( http::message<isRequest, Body, Fields>&& msg )->void;

		α OnWrite( bool close, beast::error_code ec, size_t bytes_transferred )ι->void;
		α DoClose()ι->void;
		β HandleRequest( string&& target, flat_map<string,string>&& params, up<SessionInfo> pSessionInfo, http::request<http::string_body>&& req, sp<ISession> s )ι->void=0;
		α HandleRequest( http::request<http::string_body>&& req, sp<ISession> s )ι->Task;

		α Error( Exception&& e, http::request<http::string_body>&& req )ι->void;
		α Error( http::status status, string what, http::request<http::string_body>&& req )ι->void;
		α Error( const IRequestException& e, string what, http::request<http::string_body>&& req )ι->void;
		α Send( string value, http::request<http::string_body>&& req )ι->void;
		α SendOptions( http::request<http::string_body>&& req )ι->void;

		β MakeShared()ι->sp<ISession> = 0;

		α SendQuery( sv query, UserPK userId, http::request<http::string_body>&& req, sp<ISession> s )ι->Task;
		α AddSession( UserPK userId )ι->sp<SessionInfo>;
		Ω FetchSessionInfo( SessionPK sessionId )->SessionInfoAwait{ return SessionInfoAwait{ sessionId }; }
	private:
	  template<bool isRequest, class Body, class Fields>
		α Send( http::message<isRequest, Body, Fields>&& msg )->void;

    beast::tcp_stream Stream;
    beast::flat_buffer Buffer;
    http::request<http::string_body> Request;
    sp<void> Message;
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

	template<class TSession>
	struct TListener : IListener, std::enable_shared_from_this<TListener<TSession>>
	{
	  TListener( PortType port )ε:IListener{ port }{}
		β CreateSession( tcp::socket&& socket )ι->sp<ISession> override;
		α MakeShared()ι->sp<void> override{ return this->shared_from_this(); }
	};

  template<bool isRequest, class Body, class Fields>
	α ISession::Send( http::message<isRequest, Body, Fields>&& msg )->void
	{
		auto sp = ms<http::message<isRequest, Body, Fields>>( move(msg) );
    Message = sp;
    http::async_write( Stream, *sp, beast::bind_front_handler(&ISession::OnWrite, MakeShared(), sp->need_eof()) );
	}

	template<class TSession>
  α TListener<TSession>::CreateSession( tcp::socket&& socket )ι->sp<ISession>
  {
		return  ms<TSession>( move(socket) );
  }

}
#pragma once
#include "../usings.h"

namespace Jde::Web::Rest{
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace net = boost::asio;
	using tcp = boost::asio::ip::tcp;
	struct IRestException;

	α RequestTag()ι->sp<LogTag>;
	α ResponseTag()ι->sp<LogTag>;

	struct IRestSession;
	struct Request final{
		α ClientRequest()Ι->const http::request<http::string_body>&;
		α UserId()Ι->UserPK{ return SessionInfoPtr ? SessionInfoPtr->user_id() : 0; }
		α Method()Ι->http::verb{ return ClientRequest().method(); }
		α Body()Ε->json;

		sp<IRestSession> Session;
		up<SessionInfo> SessionInfoPtr;
	};

	struct ΓW SessionInfoAwait final : IAwaitCache{
		SessionInfoAwait( SessionPK sessionId, tcp::endpoint userEndpoint, SRCE )ι:IAwaitCache{sl}, _sessionId{sessionId}{}
		α await_ready()ι->bool override;
		α await_suspend( HCoroutine h )ι->void override;
	private:
		AwaitResult _result;
		const SessionPK _sessionId;
		const tcp::endpoint _userEndpoint;
	};

	struct ΓW IRestSession{
		using TMessage = http::message<true, http::string_body, http::basic_fields<std::allocator<char>>>;
    IRestSession( tcp::socket&& socket )ι: _stream{move(socket)}//,_send{*this}
    {}

    α Run()ι->void{ net::dispatch( _stream.get_executor(), beast::bind_front_handler( &IRestSession::DoRead, MakeShared()) ); }
		α DoRead()ι->void;
    α OnRead( beast::error_code ec, size_t bytes_transferred )ι->void;

		α ClientRequest()Ι->const http::request<http::string_body>&{ return _request; }
		α OnWrite( bool close, beast::error_code ec, size_t bytes_transferred )ι->void;
		α DoClose()ι->void;
		β HandleRequest( string&& target, flat_map<string,string>&& params, Request&& req )ι->void=0;
		α HandleRequest( Request req )ι->Task;

		Ω Send( IException&& e, Request&& req )ι->void;
		Ω Send( http::status status, string what, Request&& req )ι->void;
		Ω Send( const IRestException&& e, Request&& req )ι->void;
		Ω Send( string&& value, Request&& req )ι->void;
		Ω Send( const json& payload, Request&& req )ι->void;
		Ω SendOptions( Request&& req )ι->void;

		β MakeShared()ι->sp<IRestSession> = 0;

		α SendQuery( string&& query, Request&& req )ι->Task;
		Ω AddSession( UserPK userId )ι->sp<SessionInfo>;
		Ω QuerySessions( SessionPK sessionId )ι->vector<sp<SessionInfo>>;
		Ω GetNewSessionId()ι->SessionPK;
		Ω FetchSessionInfo( SessionPK sessionId, tcp::endpoint userEndpoint )ι->SessionInfoAwait{ return SessionInfoAwait{ sessionId, userEndpoint }; }
	private:
	  template<bool isRequest, class Body, class Fields>
		Ω Send( sp<http::message<isRequest, Body, Fields>>&& m, sp<IRestSession>&& s )->void;

    beast::tcp_stream _stream;
    beast::flat_buffer _buffer;
    http::request<http::string_body> _request;
    sp<void> _message;
	};

	Ξ Request::ClientRequest()Ι->const http::request<http::string_body>&{ ASSERT(Session); return Session->ClientRequest(); }

  template<bool isRequest, class Body, class Fields>
	α IRestSession::Send( sp<http::message<isRequest, Body, Fields>>&& m, sp<IRestSession>&& s )->void{
    s->_message = m;
		auto stream = move( s->_stream );//want to move s to clear.
    http::async_write( stream, *m, beast::bind_front_handler(&IRestSession::OnWrite, move(s), m->need_eof()) );
	}
}

#pragma once
#include <boost/unordered/concurrent_flat_map.hpp>
#include "../usings.h"
#include "../exports.h"
#include "ClientSocketStream.h"
#include "ClientSocketAwait.h"
#include <jde/fwk/co/Await.h>
#include <jde/fwk/io/protobuf.h>

namespace Jde::Web::Client{
	struct IClientSocketSession;
	struct CreateClientSocketSessionAwait final : VoidAwait{
		using base = VoidAwait;
		CreateClientSocketSessionAwait( sp<IClientSocketSession> session, string host, PortType port, SRCE )ι;
		α Suspend()ι->void override;
	private:
		sp<IClientSocketSession> _session; string _host; PortType _port;
	};

	struct CloseClientSocketSessionAwait final : VoidAwait{
		using base = VoidAwait;
		CloseClientSocketSessionAwait( sp<IClientSocketSession> session, SRCE )ι:base{sl}, _session{session}{};
		α Suspend()ι->void override;
	private:
		sp<IClientSocketSession> _session;
	};

	//TODO check what should be protected
	struct ΓWC IClientSocketSession : std::enable_shared_from_this<IClientSocketSession>{
		IClientSocketSession( sp<net::io_context> ioc, optional<ssl::context>& ctx )ι;// Resolver and socket require an io_context
		α AddTask( RequestId requestId, std::any hCoroutine )ι->void;
		α PopTask( RequestId requestId )ι->std::any;

		α Run( string host, PortType port, CreateClientSocketSessionAwait::Handle h )ι->void;// Start the asynchronous operation
		α RunSession( string host, PortType port )ι{ return CreateClientSocketSessionAwait{shared_from_this(), host, port}; }
		β Query( string&& query, jobject variables, bool returnRaw, SRCE )ι->ClientSocketAwait<jvalue> = 0;
		β Subscribe( string&& query, jobject variables, sp<QL::IListener> listener, SRCE )ε->ClientSocketAwait<jarray> = 0;
		α OnMessage( string&& j, RequestId requestId )ι->void;
		α Write( string&& m )ι->void;
		α NextRequestId()ι->uint32;
		α SessionId()ι->SessionPK{ return _sessionInfo ? _sessionInfo->session_id() : SessionPK{}; }
		α SetInfo( Web::FromServer::SessionInfo info )ι->void{ _sessionInfo = move(info); }
		[[nodiscard]] α Close()ι{ return CloseClientSocketSessionAwait(shared_from_this()); }
		α Host()Ι->str{ return _host; }
		α Id()ι->uint32{ return _id; }
	protected:
		α CloseTasks( function<void(std::any&&)> f )ι->void;
		β OnClose( beast::error_code ec )ι->void;
		α IsSsl()Ι->bool{ return _stream->IsSsl(); }
		α SetId( uint32 id )ι{ _id=id; }
		ψ LogRead( const fmt::format_string<Args const&...> m, Args&&... args )ι->void;
	private:
		α OnResolve( beast::error_code ec, tcp::resolver::results_type results )ι->void;
		α OnConnect( beast::error_code ec, tcp::resolver::results_type::endpoint_type ep )ι->void;
		α OnSslHandshake(beast::error_code ec )ι->void;
		α OnHandshake( beast::error_code ec )ι->void;
		α OnRead( beast::error_code ec, uint bytes_transferred )ι->void;
		β OnReadData( std::basic_string_view<uint8_t> transmission )ι->void=0;

		tcp::resolver _resolver;
		sp<ClientSocketStream> _stream;
		string _host;
		sp<net::io_context> _ioContext;
		optional<Web::FromServer::SessionInfo> _sessionInfo;
		CreateClientSocketSessionAwait::Handle _connectHandle;
		CloseClientSocketSessionAwait::Handle _closeHandle;
		boost::concurrent_flat_map<RequestId,std::any> _tasks;
		atomic<uint32> _id;//_serverSocketIndex

		friend struct ClientSocketStream; friend struct CloseClientSocketSessionAwait;
	};

	template<class TFromClientMsgs, class TFromServerMsgs>
	struct TClientSocketSession : IClientSocketSession{
		using base=IClientSocketSession;
		TClientSocketSession( sp<net::io_context> ioc, optional<ssl::context>& ctx )ι:IClientSocketSession{ ioc, ctx }{}
		α Write( TFromClientMsgs&& m )ι->void;
	protected:
		α OnReadData( std::basic_string_view<uint8_t> transmission )ι->void override;
		β OnRead( TFromServerMsgs&& m )ι->void=0;
	};

	#define $ template<class TFromClientMsgs, class TFromServerMsgs> α TClientSocketSession<TFromClientMsgs,TFromServerMsgs>
	$::Write( TFromClientMsgs&& m )ι->void{ base::Write( Protobuf::ToString(m) ); }
	$::OnReadData( std::basic_string_view<uint8_t> transmission )ι->void{
		try{
			sv x{ (char*)transmission.data(), transmission.size() };
			auto proto = Protobuf::Deserialize<TFromServerMsgs>( transmission.data(), (int)transmission.size() );
			OnRead( move(proto) );
		}
		catch( IException& e ){
			e.SetTags( ELogTags::SocketClientRead );
		}
	}

	ψ IClientSocketSession::LogRead( const fmt::format_string<Args const&...> m, Args&&... args )ι->void{
		TRACET( ELogTags::SocketClientRead, std::forward<const fmt::format_string<Args const&...>>(m), std::forward<Args>(args)... );
	}
}
#undef $
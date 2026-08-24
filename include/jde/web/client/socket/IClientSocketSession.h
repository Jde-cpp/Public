#pragma once
#include <boost/unordered/concurrent_flat_map.hpp>
#include "../exports.h"
#include "ClientSocketStream.h"
#include "ClientSocketAwait.h"
#include "jde/fwk/process/process.h"
#include <jde/fwk/co/Await.h>
#include <jde/fwk/co/Timer.h>
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
		CloseClientSocketSessionAwait( sp<IClientSocketSession> session, bool terminate, SRCE )ι:base{sl}, _session{session}, _terminate{terminate}{};
		α await_ready()ι->bool override;
		α Suspend()ι->void override;
	private:
		sp<IClientSocketSession> _session;
		bool _terminate;
	};

	//TODO check what should be protected
	struct ΓWC IClientSocketSession : IShutdown, std::enable_shared_from_this<IClientSocketSession>{
		IClientSocketSession( sp<net::io_context> ioc, optional<ssl::context>& ctx )ι;// Resolver and socket require an io_context
		virtual ~IClientSocketSession()=default;
		α Shutdown( bool terminate, SL sl )ι->void override;
		α AddTask( RequestId requestId, std::any hCoroutine )ι->void;
		//started per request alongside AddTask: if nothing has answered by the deadline the caller would otherwise wait forever.
		α AddTimeout( RequestId requestId, SRCE )ι->TimerAwait::Task;
		α PopTask( RequestId requestId )ι->std::any;

		α Run( string host, PortType port, CreateClientSocketSessionAwait::Handle h )ι->void;// Start the asynchronous operation
		α RunSession( string host, PortType port )ι{ return CreateClientSocketSessionAwait{shared_from_this(), host, port}; }
		β Query( string&& query, jobject variables, bool returnRaw, SRCE )ι->ClientSocketAwait<jvalue> = 0;
		β Subscribe( string&& query, jobject variables, sp<QL::IListener> listener, SRCE )ε->ClientSocketAwait<jarray> = 0;
		β Unsubscribe( vector<QL::SubscriptionId>&& ids, SRCE )ι->void=0;
		α Write( string&& m )ι->void;
		α NextRequestId()ι->uint32;
		α SessionId()ι->SessionPK{ return _sessionInfo ? _sessionInfo->session_id() : SessionPK{}; }
		α SetInfo( Web::FromServer::SessionInfo&& info )ι->void{ _sessionInfo = move(info); }
		α UserPK()Ι->UserPK{ return  { _sessionInfo ? _sessionInfo->user_pk() : 0}; }
		[[nodiscard]] α Close( bool terminate, SL sl )ι{ return CloseClientSocketSessionAwait(shared_from_this(), terminate, sl); }
		α Host()Ι->str{ return _host; }
		α Id()ι->uint32{ return _id; }
	protected:
		β CloseTasks( beast::error_code ec )ι->void = 0;
		//C6: no per-request failure is possible here - the typed handle lives in the derived session's any_cast - so a request
		//that can never be answered drops the session instead, and OnClose fails every pending task through CloseTasks.  The app
		//client reconnects on close, so this is recoverable rather than fatal.
		α CloseOnError( string reason, SRCE )ι->void;
		α HasTask( RequestId requestId )Ι->bool{ return _tasks.contains( requestId ); }
		α CloseTasks( function<void(std::any&&)> f )ι->void;
		β OnClose( beast::error_code ec )ι->void;
		//mirrors the server's StreamPtr (IWebsocketSession.h): _stream is written by OnClose on the strand and read from other
		//threads - Write from any caller, Close from a shutdown thread - so always take a copy through here, never touch the
		//member directly, and treat null as "already closed".
		α StreamPtr()Ι->sp<ClientSocketStream>{ lg _{ _streamMutex }; return _stream; }
		α IsSsl()Ι->bool{ auto stream = StreamPtr(); return stream && stream->IsSsl(); }
		α SetId( uint32 id )ι{ _id=id; }
		ψ LogRead( const fmt::format_string<Args const&...> m, Args&&... args )ι->void;
	private:
		α OnResolve( beast::error_code ec, tcp::resolver::results_type results )ι->void;
		α OnConnect( beast::error_code ec, tcp::resolver::results_type::endpoint_type ep )ι->void;
		α OnSslHandshake(beast::error_code ec )ι->void;
		α OnHandshake( beast::error_code ec )ι->void;
		α OnRead( beast::error_code ec, uint bytes_transferred )ι->void;
		β OnReadData( std::span<uint8_t> transmission )ι->void=0;

		tcp::resolver _resolver;
		mutable std::mutex _streamMutex;
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
		α OnReadData( std::span<uint8_t> transmission )ι->void override;
		β OnRead( TFromServerMsgs&& m )ι->void=0;
	};

	#define $ template<class TFromClientMsgs, class TFromServerMsgs> α TClientSocketSession<TFromClientMsgs,TFromServerMsgs>
	$::Write( TFromClientMsgs&& m )ι->void{ base::Write( Protobuf::ToString(m) ); }
	$::OnReadData( std::span<uint8_t> transmission )ι->void{
		try{
			auto proto = Protobuf::Deserialize<TFromServerMsgs>( transmission.data(), transmission.size() );
			OnRead( move(proto) );
		}
		catch( Exception& e ){
			//The requestId lives inside the payload that just failed to parse, so there is no way to tell which caller was waiting
			//on it.  Swallowing it left every pending request hanging until the socket happened to close; an undecodable
			//transmission means the stream is no longer trustworthy, so drop it and let CloseTasks fail them all.
			e.SetTags( ELogTags::SocketClientRead );
			base::CloseOnError( Ƒ("undecodable transmission ({} bytes): {}", transmission.size(), e.what()) );
		}
	}

	ψ IClientSocketSession::LogRead( const fmt::format_string<Args const&...> m, Args&&... args )ι->void{
		TRACET( ELogTags::SocketClientRead, std::forward<const fmt::format_string<Args const&...>>(m), std::forward<Args>(args)... );
	}
}
#undef $
#pragma once
#include <boost/unordered/concurrent_flat_map.hpp>
#include "usings.h"
#include "exports.h"
#include "ClientStreams.h"
#include "../../../Framework/source/io/ProtoUtilities.h"
#include "../../../Framework/source/Stopwatch.h"
#include <FromServer.pb.h>

namespace Jde::Http{
	α IncomingTag()ι->sp<LogTag>;
	struct IClientSocketSession;
	struct CreateClientSocketSessionAwait final : TAwait<void,VoidTask>{
		using base = TAwait<void,VoidTask>;
		CreateClientSocketSessionAwait( sp<IClientSocketSession> session, string host, PortType port, SRCE )ι;
		α await_suspend( THandle h )ι->void override;
		α await_resume()ι->void override;
	private:
		sp<IClientSocketSession> _session;
		string _host;
		PortType _port;
		SL _sl;
	};
	struct ΓH IClientSocketSession : std::enable_shared_from_this<IClientSocketSession>{
		IClientSocketSession( sp<net::io_context> ioc, optional<ssl::context>& ctx )ι;// Resolver and socket require an io_context
		α AddTask( RequestId requestId, std::any hCoroutine )ι->void;
		α GetTask( RequestId requestId )ι->std::any;
		α CloseTasks( function<void(std::any&&)> f )ι->void;

		α Run( string host, PortType port, coroutine_handle<CreateClientSocketSessionAwait::TPromise> h )ι->void;// Start the asynchronous operation
		α RunSession( string host, PortType port )ι{ return CreateClientSocketSessionAwait{shared_from_this(), host, port}; }

		α HandleAck( const Proto::FromServer::Ack& ack )ι{ _sessionId = ack.session_id(); }
		//α Write( google::protobuf::MessageLite& m )ε->void{ Write( IO::Proto::ToString(m) ); }
		α Write( string&& m )ι->void;
		α NextRequestId()ι->uint32;
		//α OnWrite( beast::error_code ec, uint bytes_transferred )ι->void;
		α SessionId()ι->SessionPK{ return _sessionId; }
		α Close()ι->void{ _stream->Close( shared_from_this() ); }
	protected:
		β OnClose( beast::error_code ec )ι->void;
	private:
		α OnResolve( beast::error_code ec, tcp::resolver::results_type results )ι->void;
		α OnConnect( beast::error_code ec, tcp::resolver::results_type::endpoint_type ep )ι->void;
		α OnSslHandshake(beast::error_code ec )ι->void;
		α OnHandshake( beast::error_code ec )ι->void;
		α OnRead( beast::error_code ec, uint bytes_transferred )ι->void;
		β OnReadData( std::basic_string_view<uint8_t> transmission )ι->void=0;

		tcp::resolver _resolver;
		sp<HttpSocketStream> _stream;
		Stopwatch _readTimer;
		string _host;
		sp<net::io_context> _ioContext;
		SessionPK _sessionId{};
		coroutine_handle<CreateClientSocketSessionAwait::TPromise> _connectPromise;
		boost::concurrent_flat_map<RequestId,std::any> _tasks;
		friend struct HttpSocketStream;
	};

	template<class TFromClientMsgs, class TFromServerMsgs>
	struct TClientSocketSession : IClientSocketSession{
		TClientSocketSession( sp<net::io_context> ioc, optional<ssl::context>& ctx )ι:IClientSocketSession{ ioc, ctx }{}
	protected:
		α OnReadData( std::basic_string_view<uint8_t> transmission )ι->void override;
		β OnRead( TFromServerMsgs&& m )ι->void=0;
		α Write( TFromClientMsgs&& m )ε->void;
		//[[nodiscard]] α Write( TFromClientMsgs&& m, ICheckRequestId&& checkRequestId )ι{ return ClientSocketAwait<TFromServerMsgs>{ move(m), move(checkRequestId); shared_from_this() }; }
		//α WriteRequestId( TFromClientMsgs&& m )ι;
	};

	#define $ template<class TFromClientMsgs, class TFromServerMsgs> α TClientSocketSession<TFromClientMsgs,TFromServerMsgs>
	$::Write( TFromClientMsgs&& m )ε->void{
		Write( IO::Proto::ToString(m) );
	}
	$::OnReadData( std::basic_string_view<uint8_t> transmission )ι->void{
		try{
			sv x{ (char*)transmission.data(), transmission.size() };
			auto proto = IO::Proto::Deserialize<TFromServerMsgs>( transmission.data(), transmission.size() );
			OnRead( move(proto) );
		}
		catch( IException& e ){
			e.SetTag( IncomingTag() );
		}
	}
}
#undef $
#undef var
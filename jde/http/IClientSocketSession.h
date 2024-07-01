#pragma once
#include "usings.h"
#include "exports.h"
#include "ClientStreams.h"
#include "../../../Framework/source/io/ProtoUtilities.h"
#include "../../../Framework/source/Stopwatch.h"
#include <FromServer.pb.h>

namespace Jde::Http{
	α IncomingTag()ι->sp<LogTag>;

	struct ΓH IClientSocketSession : std::enable_shared_from_this<IClientSocketSession>{
		IClientSocketSession( sp<net::io_context> ioc, optional<ssl::context>& ctx )ι;// Resolver and socket require an io_context
		α Run( string host, PortType port )ι->void;// Start the asynchronous operation
		α HandleAck( const FromServer::Ack& ack )ι{ _sessionId = ack.session_id(); }
	private:
		α OnResolve( beast::error_code ec, tcp::resolver::results_type results )ι->void;
		α OnConnect( beast::error_code ec, tcp::resolver::results_type::endpoint_type ep )ι->void;
		α OnSslHandshake(beast::error_code ec )ι->void;
		α OnHandshake( beast::error_code ec )ι->void;
		α OnWrite( beast::error_code ec, uint bytes_transferred )ι->void;
		α OnRead( beast::error_code ec, uint bytes_transferred )ι->void;
		β OnReadData( std::basic_string_view<uint8_t> transmission )ι->void=0;
		α OnClose( beast::error_code ec )ι->void;
		α Write( string&& m )ι->void;
		tcp::resolver _resolver;
		HttpSocketStream _stream;
		Stopwatch _readTimer;
		string _host;
		sp<net::io_context> _ioContext;
		SessionPK _sessionId{};
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

	α NextRequestId()ι->uint32;
	#define $ template<class TFromClientMsgs, class TFromServerMsgs> α TClientSocketSession<TFromClientMsgs,TFromServerMsgs>
	$::Write( TFromClientMsgs&& m )ε->void{
		Write( IO::Proto::ToString(m) );
	}
	$::OnReadData( std::basic_string_view<uint8_t> transmission )ι->void{
		try{
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
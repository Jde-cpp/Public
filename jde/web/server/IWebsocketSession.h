#pragma once
#include <jde/App.h>
#include <jde/web/server/exports.h>
//#include <jde/web/socket/WebsocketServer.h>
//#include "../../../../Framework/source/io/Socket.h"
#include "../../../../Framework/source/io/ProtoUtilities.h"
#include "../../../../Framework/source/threading/Mutex.h"
#include "Streams.h"

#define var const auto
#define _logTag WebsocketRequestTag()

namespace Jde::Web::Server{
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace websocket = beast::websocket;
	namespace net = boost::asio;
	using tcp = net::ip::tcp;
	//using namespace Jde::IO::Sockets;
	ΓWS α SocketServerReadTag()ι->sp<Jde::LogTag>;
	ΓWS α SocketServerWriteTag()ι->sp<Jde::LogTag>;

	struct ΓWS IWebsocketSession /*abstract*/: std::enable_shared_from_this<IWebsocketSession>{
		IWebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType request, tcp::endpoint&& userEndpoint, uint32 connectionIndex )ι;
		α Run()ι->void;
		α Id()Ι{ return _id; }
		α LogWrite( string&& what, RequestId requestId, ELogLevel level=ELogLevel::Trace, SRCE )ι->void;
	protected:
		sp<SocketStream> Stream;
		tcp::endpoint _userEndpoint;
		β Close()ι->void{ Stream->Close( shared_from_this() ); }
		β OnClose()ι->void;
		β OnRead( const char* p, uint size )ι->void=0;
		β SendAck( uint id )ι->void=0;

		α LogRead( string&& what, RequestId requestId, ELogLevel level=ELogLevel::Trace, SRCE )ι->void;
		α LogWriteException( const IException& e, RequestId requestId, ELogLevel level=ELogLevel::Debug, SRCE )ι->void;
		α SessionId()ι{ return _sessionId; } α SetSessionId( SessionPK sessionId )ι{ _sessionId = sessionId; }
	private:
		α Disconnect( CodeException&& e )ι{ OnDisconnect(move(e)); /*_connected = false; _server.RemoveSession( Id );*/ }
		β OnDisconnect( CodeException&& )ι->void{}
		β OnAccept( beast::error_code ec )ι->void;

		α OnRun()ι->void;
		α DoRead()ι->void;
		α OnWrite( beast::error_code ec, std::size_t bytes_transferred )ι->void;
		TRequestType _initialRequest;
		const uint _id{};
		SessionPK _sessionId{};
		friend struct SocketStream;
	};

	template<class TFromServer, class TFromClient>
	struct TWebsocketSession /*abstract*/ : IWebsocketSession{
		TWebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType request, tcp::endpoint userEndpoint, uint32 connectionIndex )ι :
			IWebsocketSession{ move(stream), move(buffer), move(request), move(userEndpoint), connectionIndex }{}

		α OnRead( const char* p, uint size )ι->void;
		β OnRead( TFromClient&& transmission )ι->void = 0;
		α Write( TFromServer&& message )ι->void;
		α UserId()Ι{ return _userId; }
	protected:
		β WriteException( IException&& e )ι->void=0;
	private:
		uint32 _userId{};
	};

#define $ template<class TFromServer, class TFromClient> auto TWebsocketSession<TFromServer,TFromClient>
	$::OnRead( const char* p, uint size )ι->void{
		try{
			auto t = IO::Proto::Deserialize<TFromClient>( (const google::protobuf::uint8*)p, (int)size );
			OnRead( move(t) );
		}
		catch( IException& e ){
			WriteException( move(e) );
		}
	}

	$::Write( TFromServer&& message )ι->void{
		Stream->Write( message.SerializeAsString() );
	}
}
#undef $
#undef var
#undef _logTag

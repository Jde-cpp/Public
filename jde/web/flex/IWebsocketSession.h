#pragma once
#include <jde/App.h>
#include <jde/web/exports.h>
//#include <jde/web/socket/WebsocketServer.h>
#include "../../../../Framework/source/io/Socket.h"
#include "../../../../Framework/source/io/ProtoUtilities.h"
#include "../../../../Framework/source/threading/Mutex.h"
#include "Streams.h"

#define var const auto
#define _logTag WebsocketRequestTag()

namespace Jde::Web::Flex{
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace websocket = beast::websocket;
	namespace net = boost::asio;
	using tcp = net::ip::tcp;
	using namespace Jde::IO::Sockets;
	ΓW α WebsocketRequestTag()ι->sp<Jde::LogTag>;

	struct ΓW IWebsocketSession /*abstract*/: std::enable_shared_from_this<IWebsocketSession>{
		IWebsocketSession( RestStream&& stream, beast::flat_buffer&& buffer, TRequestType request )ι;
		β Close()ι->void{};
		β Run()ι->void;
	protected:
		α Disconnect( CodeException&& e )ι{ OnDisconnect(move(e)); /*_connected = false; _server.RemoveSession( Id );*/ }
		β OnDisconnect( CodeException&& )ι->void{}
		β OnAccept( beast::error_code ec )ι->void;

		SocketStream Stream;
	private:
		α OnRun()ι->void;
		α DoRead()ι->void;
		β OnRead( const char* p, uint size )ι->void=0;
		α OnWrite( beast::error_code ec, std::size_t bytes_transferred )ι->void;
		TRequestType _initialRequest;
		const uint _id{};
		friend struct SocketStream;
	};

	template<class TFromServer, class TFromClient>
	struct TWebsocketSession /*abstract*/ : IWebsocketSession{
		TWebsocketSession( RestStream&& stream, beast::flat_buffer&& buffer, TRequestType request )ε : IWebsocketSession{ move(stream), move(buffer), move(request) }{}

		α OnRead( const char* p, uint size )ι->void;
		β OnRead( TFromClient&& transmission )ι->void = 0;
		α Write( TFromServer&& message )ε->void;
		//α Write( up<string> data )ι->Task;
		α UserId()Ι{ return _userId; }
	private:
		uint32 _userId{};
	};

#define $ template<class TFromServer, class TFromClient> auto TWebsocketSession<TFromServer,TFromClient>
	$::OnRead( const char* p, uint size )ι->void{
		//DBG( "p='{:x}', size={}", (uint)p, size );
		try{
			auto t = IO::Proto::Deserialize<TFromClient>( (const google::protobuf::uint8*)p, (int)size );
			OnRead( move(t) );
		}
		catch( const IException& )
		{}
	}

	$::Write( TFromServer&& message )ε->void{
		Stream.Write( IO::Proto::ToString(message) );
	}
}
#undef $
#undef var
#undef _logTag

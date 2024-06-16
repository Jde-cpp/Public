#pragma once
#include <jde/Exports.h>
#include <jde/web/exports.h>
#include "google/protobuf/message.h"
#include "../../../../Framework/source/io/ProtoUtilities.h"

#define var const auto

namespace Jde::IO::Sockets{
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace websocket = beast::websocket;
	namespace net = boost::asio;
	using tcp = net::ip::tcp;
//	using namespace Jde::IO::Sockets;

	struct ΓW ISocketSession : std::enable_shared_from_this<ISocketSession>{
		ISocketSession( tcp::socket&& socket, SessionPK id )ι;
		virtual ~ISocketSession()=default;
		SessionPK Id;
		β OnDisconnect()ι->void=0;
	protected:
		α ReadHeader()ι->void;
		α Write( up<google::protobuf::uint8[]> p, uint c )ι->void;
		Ω LogTag()ι->sp<Jde::LogTag>;
		tcp::socket _socket;
	private:
		β ReadBody( uint messageLength )ι->void=0;
		char _readMessageSize[4];
	};

	template<class TToServer, class TFromServer>
	struct TSocketSession: public ISocketSession{
		TSocketSession( tcp::socket&& socket, SessionPK id )ι:ISocketSession{ move(socket), id } {}
	protected:
		β OnReceive( TToServer&& pValue )ε->void=0;
		α ReadBody( uint messageLength )ι->void override;
		α Write( TFromServer&& message )ι->void;
		vector<google::protobuf::uint8> _message;
	};

#define $ template<class TToServer, class TFromServer> auto TSocketSession<TToServer,TFromServer>::
//TODO consolidate with ProtoClientSession::ReadBody
	$ ReadBody( uint messageLength )ι->void{
		google::protobuf::uint8 buffer[4096];
		var useHeap = messageLength>sizeof(buffer);
		var pData = useHeap ? up<google::protobuf::uint8[]>{ new google::protobuf::uint8[messageLength] } : up<google::protobuf::uint8[]>{};
		auto pBuffer = useHeap ? pData.get() : buffer;
		try{
			var length = net::read( _socket, net::buffer(reinterpret_cast<void*>(pBuffer), messageLength) ); THROW_IF( length!=messageLength, "'{}' read!='{}' expected", length, messageLength );
			OnReceive( IO::Proto::Deserialize<TToServer>(pBuffer, (int)length) );
			ReadHeader();
		}
		catch( boost::system::system_error& e ){
			var _logTag = LogTag();
			ERR( "Read Body Failed - {}", e.what() );
			_socket.close();
		}
		catch( const IException& )
		{}
	}

	$ Write( TFromServer&& value )ι->void{
		auto [p,size] = IO::Proto::SizePrefixed( move(value) );
		ISocketSession::Write( move(p), size );
	}
#undef $
}
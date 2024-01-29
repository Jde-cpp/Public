#pragma once
#include <jde/Exports.h>
#include "google/protobuf/message.h"
#include "Exports.h"

#define var const auto

namespace Jde::IO::Sockets{
	struct ProtoSession;

	#define Φ ΓW auto
	struct ProtoServer : ISocket{
		ΓW ProtoServer( PortType defaultPort )ι;
		ΓW virtual ~ProtoServer();
		β CreateSession( tcp::socket&& socket, SessionPK id )ι->up<ProtoSession> =0;
		α RemoveSession( SessionPK id )ι{ ul _{_mutex}; _sessions.erase(id); }

	protected:
		Φ Accept()ι->void;
		std::atomic<SessionPK> _id{0};
		sp<AsioContextThread> _pIOContext;
		tcp::acceptor _acceptor;
		flat_map<SessionPK,sp<ProtoSession>> _sessions; mutable std::shared_mutex _mutex;
	private:
		void Run()ι;
	};

	struct ΓW ProtoSession{
		ProtoSession( tcp::socket&& socket, SessionPK id )ι;
		virtual ~ProtoSession()=default;
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

#pragma region TProtoSession
#define $ template<class TToServer, class TFromServer> auto TProtoSession<TToServer,TFromServer>::
	template<class TToServer, class TFromServer>
	struct TProtoSession: public ProtoSession{
		TProtoSession( tcp::socket&& socket, SessionPK id )ι:ProtoSession{ move(socket), id } {}
	protected:
		β OnReceive( TToServer&& pValue )ε->void=0;
		α ReadBody( uint messageLength )ι->void override;
		α Write( TFromServer&& message )ι->void;
		vector<google::protobuf::uint8> _message;
	};

//TODO consolidate with ProtoClientSession::ReadBody
	$ ReadBody( uint messageLength )ι->void{
		google::protobuf::uint8 buffer[4096];
		var useHeap = messageLength>sizeof(buffer);
		var pData = useHeap ? up<google::protobuf::uint8[]>{ new google::protobuf::uint8[messageLength] } : up<google::protobuf::uint8[]>{};
		auto pBuffer = useHeap ? pData.get() : buffer;
		try{
			var length = net::read( _socket, net::buffer(reinterpret_cast<void*>(pBuffer), messageLength) ); THROW_IF( length!=messageLength, "'{}' read!='{}' expected", length, messageLength );
			OnReceive( Proto::Deserialize<TToServer>(pBuffer, (int)length) );
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
		ProtoSession::Write( move(p), size );
	}

#pragma endregion
#undef Φ
#undef $
}
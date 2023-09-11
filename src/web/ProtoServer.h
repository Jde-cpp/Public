#pragma once
#include <jde/Exports.h>
#include "google/protobuf/message.h"

#define var const auto

namespace Jde::IO::Sockets
{
	struct ProtoSession;

	#define Φ Γ auto
	struct ProtoServer : ISocket
	{
		Γ ProtoServer( PortType defaultPort )noexcept;
		Γ virtual ~ProtoServer();
		β CreateSession( tcp::socket&& socket, SessionPK id )noexcept->up<ProtoSession> =0;
		α RemoveSession( SessionPK id )noexcept{ unique_lock l{_mutex}; _sessions.erase(id); }

	protected:
		Φ Accept()noexcept->void;
		std::atomic<SessionPK> _id{0};
		sp<AsioContextThread> _pIOContext;
		tcp::acceptor _acceptor;
		flat_map<SessionPK,up<ProtoSession>> _sessions; std::shared_mutex _mutex;
	private:
		void Run()noexcept;
	};

	struct Γ ProtoSession
	{
		ProtoSession( tcp::socket&& socket, SessionPK id )noexcept;
		virtual ~ProtoSession()=default;
		SessionPK Id;
		β OnDisconnect()noexcept->void=0;
	protected:
		α ReadHeader()noexcept->void;
		α Write( up<google::protobuf::uint8[]> p, uint c )noexcept->void;

		tcp::socket _socket;
		const static LogTag& _logLevel;
	private:
		β ReadBody( uint messageLength )noexcept->void=0;
		char _readMessageSize[4];
	};

#pragma region TProtoSession
#define $ template<class TToServer, class TFromServer> auto TProtoSession<TToServer,TFromServer>::
	template<class TToServer, class TFromServer>
	struct TProtoSession: public ProtoSession
	{
		TProtoSession( tcp::socket&& socket, SessionPK id )noexcept:ProtoSession{ move(socket), id } {}
	protected:
		β OnReceive( TToServer&& pValue )noexcept(false)->void=0;
		α ReadBody( uint messageLength )noexcept->void override;
		α Write( const TFromServer& message )noexcept->void;
		vector<google::protobuf::uint8> _message;
	};

//TODO consolidate with ProtoClientSession::ReadBody
	$ ReadBody( uint messageLength )noexcept->void
	{
		google::protobuf::uint8 buffer[4096];
		var useHeap = messageLength>sizeof(buffer);
		var pData = useHeap ? up<google::protobuf::uint8[]>{ new google::protobuf::uint8[messageLength] } : up<google::protobuf::uint8[]>{};
		auto pBuffer = useHeap ? pData.get() : buffer;
		try
		{
			var length = net::read( _socket, net::buffer(reinterpret_cast<void*>(pBuffer), messageLength) ); THROW_IF( length!=messageLength, "'{}' read!='{}' expected", length, messageLength );
			OnReceive( Proto::Deserialize<TToServer>(pBuffer, (int)length) );
			ReadHeader();
		}
		catch( boost::system::system_error& e )
		{
			ERR( "Read Body Failed - {}"sv, e.what() );
			_socket.close();
		}
		catch( const IException& )
		{}
	}

	$ Write( const TFromServer& value )noexcept->void
	{
		auto [p,size] = IO::Proto::SizePrefixed( value );
		ProtoSession::Write( move(p), size );
	}

#pragma endregion
#undef Φ
#undef $
}
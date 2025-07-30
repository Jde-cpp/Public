#pragma once
#include <jde/framework/process.h>
#include <jde/web/server/exports.h>
#include <jde/web/server/usings.h>
#include <jde/framework/io/proto.h>
#include <jde/ql/usings.h>
#include "../../../../../Framework/source/threading/Mutex.h"

namespace Jde::DB{ struct AppSchema; }
namespace Jde::Web::Server{
	struct RestStream; struct SocketStream;
	struct ΓWS IWebsocketSession : std::enable_shared_from_this<IWebsocketSession>{
		IWebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType request, tcp::endpoint&& userEndpoint, uint32 connectionIndex )ι;
		α Run()ι->void;
		α Id()Ι->SocketId{ return _id; }
		α LogWrite( string&& what, RequestId requestId, ELogLevel level=ELogLevel::Trace, SRCE )ι->void;
		α AddSubscription( string&& query, RequestId requestId, SRCE )ε->vector<QL::SubscriptionId>;
		α RemoveSubscription( vector<QL::SubscriptionId>&& ids, RequestId requestId, SRCE )ι->void;
		β WriteSubscription( const jvalue& j, RequestId requestId )ι->void=0;
		β WriteSubscriptionAck( vector<QL::SubscriptionId>&& subscriptionIds, RequestId requestId )ι->void=0;
		β WriteComplete( RequestId requestId )ι->void=0;
		β WriteException( exception&& e, RequestId requestId )ι->void=0;
		β WriteException( string&& e, RequestId requestId )ι->void=0;
		α UserPK()Ι{ return _userPK; }
	protected:
		sp<SocketStream> Stream;
		tcp::endpoint _userEndpoint;
		β Close()ι->void;
		β OnClose()ι->void;
		β OnRead( const char* p, uint size )ι->void=0;
		β SendAck( uint32 id )ι->void=0;

		α LogRead( string&& what, RequestId requestId, ELogLevel level=ELogLevel::Trace, SRCE )ι->void;
		α LogWriteException( const exception& e, RequestId requestId, ELogLevel level=ELogLevel::Debug, SRCE )ι->void;
		α LogWriteException( str e, RequestId requestId, ELogLevel level=ELogLevel::Debug, SRCE )ι->void;
		α SessionId()ι{ return _sessionId; } α SetSessionId( SessionPK sessionId )ι{ _sessionId = sessionId; }
		α Write( string&& m )ι->void;

	private:
		α Disconnect( CodeException&& e )ι{ OnDisconnect(move(e)); /*_connected = false; _server.RemoveSession( Id );*/ }
		β OnDisconnect( CodeException&& )ι->void{}
		β OnAccept( beast::error_code ec )ι->void;

		α OnRun()ι->void;
		α DoRead()ι->void;
		α OnWrite( beast::error_code ec, std::size_t bytes_transferred )ι->void;
		β Schemas()Ι->const vector<sp<DB::AppSchema>>& = 0;

		TRequestType _initialRequest;
		const SocketId _id{};
		SessionPK _sessionId{};
		sp<QL::IListener> _listener;
		Jde::UserPK _userPK{};
		friend struct SocketStream;
	};

	template<class TFromServer, class TFromClient>
	struct TWebsocketSession /*abstract*/ : IWebsocketSession{
		TWebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType request, tcp::endpoint userEndpoint, uint32 connectionIndex )ι :
			IWebsocketSession{ move(stream), move(buffer), move(request), move(userEndpoint), connectionIndex }{}

		α OnRead( const char* p, uint size )ι->void;
		β OnRead( TFromClient&& transmission )ι->void = 0;
		α Write( TFromServer&& message )ι->void;
	};

#define $ template<class TFromServer, class TFromClient> auto TWebsocketSession<TFromServer,TFromClient>
	$::OnRead( const char* p, uint size )ι->void{
		try{
			auto t = Proto::Deserialize<TFromClient>( (const google::protobuf::uint8*)p, (int)size );
			OnRead( move(t) );
		}
		catch( IException& e ){
			WriteException( move(e), RequestId{0} );
		}
	}

	$::Write( TFromServer&& message )ι->void{
		IWebsocketSession::Write( message.SerializeAsString() );
	}
}
#undef $
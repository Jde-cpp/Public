#pragma once
#include <jde/fwk/co/Timer.h>
#include <jde/fwk/process/process.h>
#include <jde/web/server/usings.h>
#include <jde/web/server/Sessions.h>
#include <jde/fwk/io/protobuf.h>
#include <jde/ql/usings.h>
#include <jde/ql/QLAwait.h>
#include <jde/fwk/co/CoLock.h>
#include "QueryClientAwait.h"

namespace Jde::DB{ struct AppSchema; }
namespace Jde::Proto{ struct Query; }
namespace Jde::QL{ struct Subscription; }

namespace Jde::Web::Server{
	struct RestStream; struct SocketStream;
	struct ΓWS IWebsocketSession : std::enable_shared_from_this<IWebsocketSession>{
		IWebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType request, tcp::endpoint&& userEndpoint, uint32 connectionIndex )ι;
		α Run()ι->void;
		α Id()Ι->SocketId{ return _id; }
		α QueryClient( QL::TableQL query, Jde::UserPK executer, SRCE )ι->QueryClientAwait{ return QueryClientAwait{move(query), executer, shared_from_this(), sl}; }
		α QueryClient( QL::TableQL&& query, Jde::UserPK executer, QueryClientAwait::Handle h, SRCE )ι->void;
		α AddSubscription( string&& query, jobject variables, RequestId requestId, SRCE )ε->flat_set<QL::SubscriptionId>;
		α LogWrite( string&& what, RequestId requestId, ELogLevel level=ELogLevel::Trace, SRCE )ι->void;
		α RemoveSubscription( vector<QL::SubscriptionId>&& ids, RequestId requestId, SRCE )ι->void;
		β WriteSubscription( const jvalue& j, RequestId requestId )ι->void=0;
		β WriteSubscription( uint32 appPK, uint32 appInstancePK, const Logging::Entry& e, const QL::Subscription& sub )ι->void=0;
		β WriteSubscriptionAck( flat_set<QL::SubscriptionId>&& subscriptionIds, RequestId requestId )ι->void=0;
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
		β Query( Proto::Query&& query, RequestId requestId, function<string(string&&, RequestId)>&& toProtoString )ι->QL::QLAwait<jvalue>::Task;

		α LogRead( string&& what, RequestId requestId, ELogLevel level=ELogLevel::Trace, SRCE )ι->void;
		α LogWriteException( const exception& e, RequestId requestId, ELogLevel level=ELogLevel::Debug, SRCE )ι->void;
		α LogWriteException( str e, RequestId requestId, ELogLevel level=ELogLevel::Debug, SRCE )ι->void;
		α QueryClientResults( string&& queryResult, RequestId requestId )ι->void;
		α Schemas()Ι->const vector<sp<DB::AppSchema>>&{ return LocalQL()->Schemas(); }
		α Session()Ι->const sp<SessionInfo>&{ return _sessionInfo; }
		α SessionId()ι{ return _sessionInfo ? _sessionInfo->SessionId : SessionPK{}; }
		α SetSessionId( SessionPK sessionId )ι->void;
		α SetSessionInfo( sp<SessionInfo> sessionInfo )ι->void{ _sessionInfo = move(sessionInfo); }
		α Write( string&& m )ι->void;

	private:
		α AddTimeout( RequestId requestId, QueryClientAwait::Handle h, Duration timeout, SRCE )ι->TimerAwait::Task;
		α Disconnect( CodeException&& e )ι{ OnDisconnect(move(e)); /*_connected = false; _server.RemoveSession(Id);*/ }
		β OnDisconnect( CodeException&& )ι->void{}
		β OnAccept( beast::error_code ec )ι->void;

		α OnRun()ι->void;
		α DoRead()ι->void;
		α OnWrite( beast::error_code ec, std::size_t bytes_transferred )ι->void;
		β QueryClient( QL::TableQL&& query, Jde::UserPK executer, RequestId requestId )ε->void=0;
		β LocalQL()Ι->sp<QL::IQL> = 0;

		const SocketId _id{};
		TRequestType _initialRequest;
		sp<QL::IListener> _listener;
		flat_map<RequestId, std::pair<QueryClientAwait::Handle, sp<DurationTimer>>> _pendingQueries; mutex _pendingQueriesMutex;
		atomic<RequestId> _requestId;
		sp<SessionInfo> _sessionInfo;
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
			auto t = Protobuf::Deserialize<TFromClient>( (const google::protobuf::uint8*)p, (int)size );
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
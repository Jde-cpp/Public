#include <jde/web/server/IWebsocketSession.h>
#include <jde/fwk/log/log.h>
#include <jde/ql/ql.h>
#include <jde/ql/LocalSubscriptions.h>
#include "ServerImpl.h"
#include <jde/web/server/SubscribeLog.h>
#include <jde/app/proto/Common.pb.h>

#define let const auto

namespace Jde::App::Proto::FromServer{ struct Traces; }
namespace Jde::Web::Server{
	//TODO comment
	struct SocketServerListener : QL::IListener{
		SocketServerListener( sp<IWebsocketSession> session )ι: QL::IListener{ Ƒ("[{}]Socket", session->Id()) }, _session{ session }{}
		α OnChange( const jvalue& j, QL::SubscriptionId clientId )ε->void{ _session->WriteSubscription(j, clientId); }
		α OnTraces( App::Proto::FromServer::Traces&& /*traces*/ )ι->void{ ASSERT(false); }
		sp<IWebsocketSession> _session;
	};

	IWebsocketSession::IWebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType request, tcp::endpoint&& userEndpoint, uint32 connectionIndex )ι:
		Stream{ ms<SocketStream>(move(stream), move(buffer)) },
		_userEndpoint{ userEndpoint },
		_id{ connectionIndex },
		_initialRequest{ move(request) }
	{}

	α IWebsocketSession::Run()ι->void{
		LogRead( "Run", 0 );
		SocketServerListener foo{ shared_from_this() };
		_listener = ms<SocketServerListener>( shared_from_this() );
		Stream->DoAccept( move(_initialRequest), shared_from_this() );
	}

#define CHECK_EC( ec,tag,  ... ) if( ec ){ CodeException(static_cast<std::error_code>(ec), tag __VA_OPT__(,) __VA_ARGS__); return; }
	α IWebsocketSession::OnAccept( beast::error_code ec )ι->void{
		LogRead( "OnAccept", 0 );
		CHECK_EC( ec, ELogTags::SocketServerRead );
		SendAck( Id() );
		DoRead();
	}

	α IWebsocketSession::DoRead()ι->void{
		Stream->DoRead( shared_from_this() );
	}

	α IWebsocketSession::Write( string&& m )ι->void{
		Stream->Write( move(m) );
	}

	α IWebsocketSession::OnWrite( beast::error_code ec, uint c )ι->void{
		boost::ignore_unused( c );
		try{
			THROW_IFX( ec, CodeException(static_cast<std::error_code>(ec), ELogTags::SocketServerWrite, ec == websocket::error::closed ? ELogLevel::Trace : ELogLevel::Error) );
		}
		catch( const CodeException& )
		{}
	}

	α IWebsocketSession::LogRead( string&& what, RequestId requestId, ELogLevel level, SL sl )ι->void{ //TODO forward args.
		Logging::Log( level, ELogTags::SocketServerRead, sl, "[{:x}.{:x}]{}", Id(), requestId, move(what) );
	}

	α IWebsocketSession::LogWrite( string&& what, RequestId requestId, ELogLevel level, SL sl )ι->void{
		Logging::Log( level, ELogTags::SocketServerWrite, sl, "[{:x}.{:x}]{}", Id(), requestId, move(what) );
	}

	α IWebsocketSession::LogWriteException( const exception& e, RequestId requestId, ELogLevel level, SL sl )ι->void{
		if( let p = dynamic_cast<const IException*>(&e); p )
			p->SetLevel( ELogLevel::NoLog );
		Exception{ sl, level, "[{}.{}]{}", Ƒ("{:x}", Id()), Ƒ("{:x}", requestId), e.what() }; //:x doesn't work with exception formatter
	}
	α IWebsocketSession::LogWriteException( str e, RequestId requestId, ELogLevel level, SL sl )ι->void{
		Exception{ sl, level, "[{}.{}]{}", Ƒ("{:x}", Id()), Ƒ("{:x}", requestId), move(e) }; //:x doesn't work with exception formatter
	}

	α IWebsocketSession::Close()ι->void{
		if( _listener ){
			QL::Subscriptions::StopListen( _listener );
			_listener = nullptr;
		}
		Stream->Close( shared_from_this() );
	}
	α IWebsocketSession::OnClose()ι->void{
		LogRead( "SererSocket::OnClose.", 0 );
		Internal::RemoveSocketSession( Id() );
		_listener = nullptr;
	}

	α IWebsocketSession::AddSubscription( string&& query, jobject variables, RequestId requestId, SL sl )ε->flat_set<QL::SubscriptionId>{
		auto subs = QL::ParseSubscriptions( move(query), variables, Schemas(), sl );
		flat_set<QL::SubscriptionId> subscriptionIds;//client ids.
		vector<QL::Subscription> logSubs;
		for( auto sub=subs.begin(); sub!=subs.end();  ){
			if( !sub->Id )
				sub->Id = requestId;
			subscriptionIds.emplace( sub->Id );
			let isLog = sub->TableName=="logs";
			if( isLog )
				logSubs.emplace_back( move(*sub) );
			sub = isLog ? subs.erase( sub ) : next( sub );
		}
		if( !logSubs.empty() )
			Logging::GetLogger<SubscribeLog>().Add( shared_from_this(), move(logSubs) );
		if( _listener )
			QL::Subscriptions::Listen( _listener, move(subs) );
		return subscriptionIds;
	}
	α IWebsocketSession::RemoveSubscription( vector<QL::SubscriptionId>&& ids, RequestId requestId, SL /*sl*/ )ι->void{
		try{
			QL::Subscriptions::StopListen( _listener, move(ids) );
		}
		catch( std::exception& e ){
			WriteException( move(e), requestId );
		}
	}
	α IWebsocketSession::Query( Proto::Query&& query, RequestId requestId, function<string(string&&, RequestId)>&& toProtoQuery )ι->QL::QLAwait<jvalue>::Task{
		let _ = shared_from_this();
		try{
			LogRead( Ƒ("GraphQL{}: {}", query.return_raw() ? "*" : "", query.text()), requestId );
			auto j = co_await QL::QLAwait<jvalue>( move(*query.mutable_text()), parse(move(*query.mutable_variables())).as_object(), _userPK, LocalQL(), query.return_raw() );
			auto y = serialize( move(j) );
			LogWrite( Ƒ("GraphQL: {}", y.substr(0,100)), requestId );
			Write( toProtoQuery(move(y), requestId) );
		}
		catch( exception& e ){
			WriteException( move(e), requestId );
		}
	}

	α IWebsocketSession::AddTimeout( RequestId requestId, QueryClientAwait::Handle h, Duration timeout, SL sl )ι->TimerAwait::Task{
		auto timer = ms<DurationTimer>( timeout, ELogTags::SocketServerWrite, sl );
		{
			lg l{ _pendingQueriesMutex };
			_pendingQueries.emplace( requestId, make_pair(h, timer) );
		}
		auto timedout = co_await *timer;
		lg l{ _pendingQueriesMutex };
		if( auto it = _pendingQueries.find(requestId); it!=_pendingQueries.end() ){
			auto h = it->second.first;
			_pendingQueries.erase( it );
			if( h ){
				h.promise().SetExp( Exception{ELogTags::SocketServerWrite, sl, "Query {} timed out after {}s", hex(requestId), Chrono::ToString(timeout)} );
				h.resume();
			}
		}
		else
			CRITICALT( ELogTags::SocketServerRead, "[{}]No pending query", hex(requestId) );
	}
	α IWebsocketSession::QueryClient( QL::TableQL&& query, Jde::UserPK executer, QueryClientAwait::Handle h, SL sl )ι->void{
		let requestId = ++_requestId;
		AddTimeout( requestId, h, 10s, sl );
		QueryClient( move(query), executer, requestId );
	}
	α IWebsocketSession::QueryClientResults( string&& queryResult, RequestId requestId )ι->void{
		LogRead( Ƒ("QueryClientResults: {}", queryResult.substr(0,100)), requestId );
		QueryClientAwait::Handle h;
		{
			lg l{ _pendingQueriesMutex };
			if( auto it = _pendingQueries.find(requestId); it!=_pendingQueries.end() ){
				h = it->second.first;
				it->second.first = nullptr;
				it->second.second->Cancel(); //will delete iterator
			}
			else
				CRITICALT( ELogTags::SocketServerRead, "[{}]No pending query", hex(requestId) );
		}
		if( h ){
			try{
				h.promise().SetValue( parse(move(queryResult)) );
			}
			catch( exception& e ){
				h.promise().SetExp( Exception{SRCE_CUR, move(e), ELogLevel::Warning, "[{}]QueryClientResults parse exception", hex(requestId)} );
			}
			h.resume();
		}
		else
			WARNT( ELogTags::SocketServerRead, "[{}]No pending query", hex(requestId) );
	}
	α IWebsocketSession::SetSessionId( SessionPK sessionId )ι->void{
		if( !_sessionInfo )
			_sessionInfo = ms<SessionInfo>();
		_sessionInfo->SessionId = sessionId;
	}
}
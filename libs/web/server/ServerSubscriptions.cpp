#include <jde/web/server/ServerSubscriptions.h>
#include <jde/web/server/IWebsocketSession.h>
#include <jde/framework/math/HiLow.h>
#include <jde/ql/ql.h>
#include <jde/ql/IQL.h>
#include <jde/ql/types/Subscription.h>

#define let const auto
namespace Jde::Web::Server{
	struct SocketRequestId final : HiLow{
		SocketRequestId( Server::SocketId h, Jde::RequestId l )ι:HiLow{ h, l }{}
		SocketRequestId( Handle x )ι:HiLow{ x }{}
		α SocketId()Ι->Server::SocketId{ return Hi(); }
		α RequestId()Ι->Jde::RequestId{ return Low(); }
	};

	struct Listener final :  QL::IListener{
		α OnChange( const jvalue& j, QL::SubscriptionClientId socketRequestId )ε->void override;
	};
	sp<Listener> _listener = sp<Listener>{};
	flat_map<SocketId,sp<IWebsocketSession>> _sockets; static shared_mutex _socketMutex;
	flat_map<SocketId,flat_set<RequestId>> _socketRequests; static shared_mutex _socketRequestMutex;

	α Listener::OnChange( const jvalue& j, QL::SubscriptionClientId subscriptionId )ε->void{
		const SocketRequestId socketRequestId{ subscriptionId };
		sl _{ _socketMutex };
		if( auto socket = _sockets.find(socketRequestId.SocketId()); socket!=_sockets.end() )
			socket->second->WriteSubscription( jvalue{j}, socketRequestId.RequestId() );
		else
			Warning( ELogTags::SocketServerWrite, "[{:x}.{:x}]Could not find socket subscription.", socketRequestId.SocketId(), socketRequestId.RequestId() );
	}

	α Subscriptions::Add( string&& query, RequestId requestId, sp<IWebsocketSession> socket, SL sl )ι->TAwait<vector<QL::SubscriptionId>>::Task{
		const SocketRequestId socketRequestId{ socket->Id(), requestId };
		try{
			auto subscriptionIds = co_await QL::Local()->Subscribe( move(query), _listener, socketRequestId, socket->UserPK(), sl );
			{
				ul _{ _socketMutex };
				_sockets.try_emplace( socketRequestId.SocketId(), socket );
			}
			ul _{ _socketRequestMutex };
			_socketRequests.try_emplace( socketRequestId.SocketId() ).first->second.emplace( socketRequestId.RequestId() );
			socket->WriteSubscriptionAck( move(subscriptionIds), requestId );
		}
		catch( IException& e ){
			socket->WriteException( move(e), requestId );
		}
	}
	α Subscriptions::Remove( vector<RequestId>&& previousRequestIds, RequestId currentRequestId, sp<IWebsocketSession> socket, SL sl )ι->VoidAwait<>::Task{
		vector<QL::SubscriptionClientId> socketRequestIds;
		let socketId = socket->Id();
		ul l{ _socketRequestMutex };
		if( auto pRequests = _socketRequests.find(socketId); pRequests!=_socketRequests.end() ){
			for( auto requestId : previousRequestIds ){
				socketRequestIds.emplace_back( SocketRequestId{socketId, requestId} );
				pRequests->second.erase( requestId );
			}
			l.unlock();
			if( pRequests->second.empty() ){
				ul _{ _socketMutex };
				_socketRequests.erase( socket->Id() );
			}
			if( !socketRequestIds.empty() )
				try{
					co_await QL::Local()->Unsubscribe( move(socketRequestIds), sl );
					socket->WriteComplete( currentRequestId );
				}
				catch( IException& e ){
					socket->WriteException( move(e), currentRequestId );
				}
			else
			socket->WriteException( Exception{"no requests found"}, currentRequestId );
		}
		else
		socket->WriteException( Exception{"no requests found"}, currentRequestId );
	}
	α Subscriptions::Close( SocketId socketId, SL sl )ι->VoidAwait<>::Task{
		{
			ul _{ _socketMutex };
			_sockets.erase( socketId );
		}
		ul l{ _socketRequestMutex };
		if( auto pRequests = _socketRequests.find(socketId); pRequests!=_socketRequests.end() ){
			vector<QL::SubscriptionClientId> socketRequestIds;
			for( auto requestId : pRequests->second )
				socketRequestIds.emplace_back( SocketRequestId{socketId, requestId} );
			_socketRequests.erase( pRequests );
			l.unlock();
			if( !socketRequestIds.empty() )
				co_await QL::Local()->Unsubscribe( move(socketRequestIds), sl );
		}
	}
}
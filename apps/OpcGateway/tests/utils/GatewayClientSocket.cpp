#include "GatewayClientSocket.h"
#include "../../src/types/proto/opc.Common.h"
#include "../../src/types/proto/opc.FromClient.h"
#include "../../src/types/UAClientException.h"
#include <jde/app/proto/common.h>

#define let const auto

namespace Jde::Opc::Gateway::Tests{
	GatewayClientSocket::GatewayClientSocket( sp<net::io_context> ioc, optional<ssl::context>& ctx )ι:
		base{ ioc, ctx }
	{}

	α GatewayClientSocket::OnAck( uint32 serverSocketId )ι->void{
		SetId( serverSocketId );
		INFOT( ELogTags::SocketClientRead, "[{}]{} GatewayClientSocket created: {}.", Id(), IsSsl() ? "Ssl" : "Plain", Host() );
	}

	α GatewayClientSocket::HandleException( std::any&& h, Jde::Proto::Exception&& e )ι{
		if( auto pEcho = std::any_cast<ClientSocketAwait<string>::Handle>(&h) ){
			pEcho->promise().SetExp( Exception{e.what(), e.code()} );
			pEcho->resume();
		}
		else if( auto pAck = std::any_cast<ClientSocketAwait<SessionPK>::Handle>(&h) ){
			pAck->promise().SetExp( Exception{e.what(), e.code()} );
			pAck->resume();
		}
		else
			WARNT( ELogTags::SocketClientRead, "Failed to process incomming exception '{}'.", e.what() );
	}

	α onNodeValues( FromServer::NodeValues&& nodeValues )ι->void;
	α onSubscriptionAck( RequestId requestId, const FromServer::SubscriptionAck& result )ι->StatusCode;
	α GatewayClientSocket::OnRead( FromServer::Transmission&& transmission )ι->void{
		auto size = transmission.messages_size();
		for( auto i=0; i<size; ++i ){
			auto m = transmission.mutable_messages( i );
			using enum FromServer::Message::ValueCase;
			let requestId = m->request_id();
			switch( m->Value_case() ){
			case kAck:
				OnAck( m->ack() );
				break;
			case kNodeValues:
				onNodeValues( move(*m->mutable_node_values()) );
				break;
			case kException:{
				std::any h = requestId==0 ? coroutine_handle<>{} : PopTask( requestId );
				HandleException( move(h), move(*m->mutable_exception()) );
				break;}
			case kQuery:{
				auto h = std::any_cast<ClientSocketAwait<jvalue>::Handle>( IClientSocketSession::PopTask(requestId) );
				try{
					h.promise().Resume( parse(move(*m->mutable_query())), h );
				}
				catch( std::exception& e ){
					h.promise().ResumeExp( move(e), h );
				}
				break;}
			case kSubscriptionAck:{
				auto& result = *m->mutable_subscription_ack();
				auto h = std::any_cast<ClientSocketAwait<FromServer::SubscriptionAck>::Handle>( IClientSocketSession::PopTask(requestId) );
				if( let sc = onSubscriptionAck(requestId, result); sc )
					h.promise().ResumeExp( UAException{sc}, h );
				else
					h.promise().Resume( move(result), h );
				break;}
			case kUnsubscribeAck:{
				auto h = std::any_cast<ClientSocketAwait<FromServer::UnsubscribeAck>::Handle>( IClientSocketSession::PopTask(requestId) );
				h.promise().Resume( move(*m->mutable_unsubscribe_ack()), h );
				break;}
			case VALUE_NOT_SET:{
				auto h = std::any_cast<ClientSocketAwait<uint32>::Handle>( IClientSocketSession::PopTask(requestId) ); //connect
				h.promise().Resume( Id(), h );
			break;}
			default:
				BREAK;
			}
		}
	}
	α GatewayClientSocket::Connect( SessionPK sessionId, SL sl )ι->await<uint32>{
		let requestId = NextRequestId();
		return await<uint32>{ FromClientUtils::Connection(requestId, sessionId), requestId, shared_from_this(), sl };
	}

	flat_map<RequestId, tuple<ServerCnnctnNK, vector<NodeId>, sp<IListener>>> _subscriptionRequests; shared_mutex _subscriptionRequestMutex;
	flat_map<ServerCnnctnNK, flat_map<NodeId, flat_set<sp<IListener>>>> _subscriptions; shared_mutex _subscriptionsMutex;
	α GatewayClientSocket::Query( string&& query, jobject variables, bool returnRaw, SL sl )ι->ClientSocketAwait<jvalue>{
		let requestId = NextRequestId();
		LOGSL( ELogLevel::Trace, sl, ELogTags::SocketClientWrite, "[{:x}]'{}', variables: {}.", requestId, query, serialize(variables) );
		return ClientSocketAwait<jvalue>{ FromClientUtils::Query(move(query), move(variables), returnRaw, requestId), requestId, shared_from_this(), sl };
	}
	α GatewayClientSocket::Subscribe( ServerCnnctnNK target, const vector<NodeId>& nodes, sp<IListener> listener, SL sl )ε->await<FromServer::SubscriptionAck>{
		let requestId = NextRequestId();
		LOGSL( ELogLevel::Trace, sl, ELogTags::SocketClientWrite, "[{:x}]Subscribe: '{}'.", requestId, target );
		ul _{ _subscriptionRequestMutex };
		_subscriptionRequests.emplace( requestId, make_tuple(target, nodes, move(listener)) );
		return await<FromServer::SubscriptionAck>{ FromClientUtils::Subscription(move(target), nodes, requestId), requestId, shared_from_this(), sl };
	}

	flat_map<SubscriptionId, sp<IListener>> _logSubscriptions; shared_mutex _logSubscriptionsMutex;
	α GatewayClientSocket::LogSubscribe( jobject&& ql, jobject vars, sp<IListener> listener, SL sl )ε->ClientSocketAwait<jarray>{
		let requestId = NextRequestId();
		ql["id"] = requestId;
		ul _{ _logSubscriptionsMutex };
		_logSubscriptions.emplace( (uint32)requestId, move(listener) );
		auto query = serialize(ql);
		LOGSL( ELogLevel::Trace, sl, ELogTags::SocketClientWrite, "[{:x}]Subscribe: '{}'.", requestId, query.substr(0, Web::Client::MaxLogLength()) );
		return ClientSocketAwait<jarray>{ FromClientUtils::Query(move(query), move(vars), true, requestId), requestId, shared_from_this(), sl };
	}
	α GatewayClientSocket::Unsubscribe( ServerCnnctnNK target, const vector<NodeId>& nodeIds, SL sl )ε->ClientSocketAwait<FromServer::UnsubscribeAck>{
		let requestId = NextRequestId();
		LOGSL( ELogLevel::Trace, sl, ELogTags::SocketClientWrite, "[{:x}]Unsubscribe: '{}'.", requestId, target );
		return ClientSocketAwait<FromServer::UnsubscribeAck>{ FromClientUtils::Unsubscription(move(target), nodeIds, requestId), requestId, shared_from_this(), sl };
	}

	α onSubscriptionAck( RequestId requestId, const FromServer::SubscriptionAck& result )ι->StatusCode{
		ul _{ _subscriptionRequestMutex };
		auto it = _subscriptionRequests.find( requestId );
		if( it==_subscriptionRequests.end() ){
			CRITICALT( ELogTags::SocketClientRead, "[{:x}]No subscription request found.", requestId );
			return UA_STATUSCODE_BADINTERNALERROR;
		}
		auto& [target, nodes, listener] = it->second;
		ul _2{ _subscriptionsMutex };
		auto& targetNodes = _subscriptions[target];
		uint resultsSize = result.results_size();
		ASSERT( nodes.size()==resultsSize );
		bool added{};
		StatusCode sc{};
		for( uint i=0; i<std::min(resultsSize, nodes.size()); ++i ){
			let& res = result.results( i );
			auto& nodeId = nodes[i];
			if( res.status_code() ){
				sc = res.status_code();
				DBGT( ELogTags::SocketClientRead, "[{:x}]Subscription for node '{}' failed with status code {}.", requestId, nodeId.ToString(), sc );
			}
			else{
				targetNodes[nodeId].emplace( listener );
				added = true;
			}
		}
		_subscriptionRequests.erase( it );
		return added ? UA_STATUSCODE_GOOD : sc;
	}

	α onNodeValues( FromServer::NodeValues&& nodeValues )ι->void{
		sl _{ _subscriptionsMutex };
		let& opcId = nodeValues.opc_id();
		auto targetNodes = _subscriptions.find( opcId );
		if( targetNodes==_subscriptions.end() ){
			DBGT( ELogTags::SocketClientRead, "No subscriptions for opcId '{}'.", opcId );
			return;
		}
		let nodeId = ProtoUtils::ToNodeId( nodeValues.node() );
		let& listeners = targetNodes->second.find( nodeId );
		if( listeners==targetNodes->second.end() ){
			DBGT( ELogTags::SocketClientRead, "[{},{}]No subscriptions.", opcId, nodeId.ToString() );
			return;
		}
		for( let& listener : listeners->second ){
			listener->OnData( opcId, nodeId, Protobuf::ToVector<FromServer::Value>(move(*nodeValues.mutable_values())) );
		}
	}

	α GatewayClientSocket::OnClose( beast::error_code ec )ι->void{
		auto f = [this, ec]( std::any&& h )->void {
			CodeException e{ ec, ELogTags::SocketClientWrite, ELogLevel::NoLog };
			HandleException( move(h), App::ProtoUtils::ToException(move(e)) );
		};
		CloseTasks( f );
		base::OnClose( ec );
	}
}
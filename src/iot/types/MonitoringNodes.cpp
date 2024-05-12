#include <jde/iot/types/MonitoringNodes.h>
#include <jde/iot/uatypes/UAClient.h>
#include "../uatypes/CreateMonitoredItemsRequest.h"

#define var const auto

namespace Jde::Iot{
	static sp<LogTag> _logTag{ Logging::Tag( "app.monitoring" ) };
	α UAMonitoringNodes::LogTag()ι->sp<Jde::LogTag>{ return _logTag; }

	//until orphaned UAClient's go away.
	α UAMonitoringNodes::Shutdown()ι->void{
		ul lock{ _mutex };
		_requests.clear();
		_calls.clear();
		_errors.clear();

		flat_map<SubscriptionId,flat_set<MonitorId>> monitoredItems;
		for( var& [h,_] : _subscriptions )
			monitoredItems.try_emplace( h.SubId() ).first->second.emplace( h.MonitorId() );
		_subscriptions.clear();
		lock.unlock();
		for( auto& [subscriptionId, monitorIds] : monitoredItems )
			_pClient->DataSubscriptionDelete( subscriptionId, move(monitorIds) );
	}

	α UAMonitoringNodes::FindNode( const NodeId& node )ι->tuple<MonitorHandle,Subscription*>{
		auto p = find_if( _subscriptions, [&node](var& x){ return x.second.Node==node;} );
		return p!=_subscriptions.end() ? make_tuple( p->first, &p->second ) : make_tuple( MonitorHandle{0,0}, nullptr );
	}

	α UAMonitoringNodes::Subscribe( sp<IDataChange>&& dataChange, flat_set<NodeId>&& nodes, HCoroutine&& h, Handle& requestId )ι->void{
		requestId = MonitorHandle{ _pClient->SubscriptionId(), ++_requestId };
		flat_set<NodeId> newNodes;
		//todo:  check for existing node subscriptions in progress.
		ul lock{ _mutex };
		for( auto n : nodes ){
			if( auto pSubscription = get<1>(FindNode(n)); pSubscription )
				pSubscription->ClientCalls.emplace( dataChange );
			else
				newNodes.emplace( move(n) );
		}
		_requests.emplace( requestId, move(nodes) );
		if( newNodes.empty() ){
			lock.unlock();
			h.resume();
		}
		else{
			_calls.emplace( requestId, make_tuple(newNodes,move(dataChange)) );
			lock.unlock();
			string nodeString;
			nodeString = accumulate( newNodes.begin(), newNodes.end(), nodeString, []( string&& s, const NodeId& n ){return s+=n.to_string()+",";} );
//			DBG( "DataSubscriptions: [{}]", nodeString.substr(0, nodeString.size()-1) );
			_pClient->DataSubscriptions( CreateMonitoredItemsRequest{move(newNodes)}, requestId, move(h) );
		}
	}
	α UAMonitoringNodes::OnCreateResponse( UA_CreateMonitoredItemsResponse* response, Handle requestId )ι->void{
		MonitorHandle requestHandle{ requestId };
		ul _{ _mutex };
		if( auto pCall = _calls.find(requestId); pCall!=_calls.end() ){
			auto& nodes = get<0>(pCall->second);
			auto& dataChange = get<1>(pCall->second);
			ASSERT( nodes.size()==response->resultsSize );
			uint i{};
			for( auto pNode = nodes.begin(); i<response->resultsSize && pNode!=nodes.end(); ++pNode, ++i ){
		  	MonitoredItemCreateResult result{ move(response->results[i]) };
				if( result.statusCode ){
					DBG( "[{:x}]Could not create monitored item for node '{}':  {}.", requestId, pNode->to_string(), UAException::Message(result.statusCode) );
					_errors.try_emplace( requestId ).first->second.try_emplace( move(*pNode), result.statusCode );
				}
				else{
					var h = MonitorHandle{ requestHandle.SubId(), result.monitoredItemId };
					TRACE( "[{:x}.{:x}]Monitoring '{}'", _pClient->Handle(), (Handle)h, pNode->to_string() );
					_subscriptions.emplace( h, Subscription{move(*pNode), move(result), dataChange} );
					if( _subscriptions.size()==1 )
						_pClient->ProcessDataSubscriptions();
				}
  		}
			_calls.erase( requestId );
		}
		else
			CRITICAL( "Could not find call for subscription='{:x}' index='{:x}'.", requestHandle.SubId(), requestHandle.MonitorId() );
	}
	α UAMonitoringNodes::GetResult( Handle requestId, StatusCode sc )ι->FromServer::SubscriptionAck{
		FromServer::SubscriptionAck y;
		ul _{ _mutex };
		flat_map<NodeId,StatusCode>* errors = _errors.find(requestId)!=_errors.end() ? &_errors[requestId] : nullptr;
		if( auto pRequest = _requests.find(requestId); pRequest!=_requests.end() ){
			for( auto& n : pRequest->second ){
				auto nodeResult = y.add_results();
				if( auto pSubscription = get<1>(FindNode(n)); pSubscription ){
					nodeResult->set_status_code( pSubscription->Result.statusCode );
					nodeResult->set_revised_sampling_interval( pSubscription->Result.revisedSamplingInterval );
					nodeResult->set_revised_queue_size( pSubscription->Result.revisedQueueSize );
				}
				else if( auto nodeSC = errors ? Find(*errors,n) : StatusCode{}; nodeSC )
					nodeResult->set_status_code( nodeSC );
				else{
					nodeResult->set_status_code( sc ? sc : UA_STATUSCODE_BADCONFIGURATIONERROR );
					if( !sc )
						CRITICAL( "[{:x}]Could not find subscription for node '{}'.", requestId, n.to_string() );
				}
			}
			_requests.erase( pRequest );
		}

		if( errors )
			_errors.erase( requestId );
		return y;
	}

	α UAMonitoringNodes::SendDataChange( Handle h, const Value&& value )ι->uint{
		sl _{ _mutex };
		uint calls = 0;
		if( auto pSubscription = _subscriptions.find(h); pSubscription!=_subscriptions.end() ){
			auto& args = pSubscription->second;
			calls = args.ClientCalls.size();
			for_each( args.ClientCalls, [&opcId=_pClient->Target(),value,&args](var& x){x->SendDataChange(opcId, args.Node, value);} );
		}
		else
			TRACE( "Could not find subscription:  {:x}.", h );
		return calls;
	}
	α UAMonitoringNodes::Unsubscribe( sp<IDataChange> dataChange )ι->void{
		DBG( "[{}]UAMonitoringNodes::Unsubscribe()", dataChange->to_string() );
		flat_set<MonitorHandle> handles;
		auto f = [&dataChange]( var& hNodeDataChange ){return get<1>(hNodeDataChange.second)==dataChange;};
		ul _{ _mutex };
		for( auto p = find_if(_calls, f); p!=_calls.end(); p=find_if(++p, _calls.end(), f) )
			handles.emplace( p->first );
		for( var& h : handles ){
			_calls.erase( h );
			_requests.erase( h );
			_errors.erase( h );
		}

		flat_map<SubscriptionId,flat_set<MonitorId>> toDelete;
		for( auto& [h,subscription] : _subscriptions ){
			if( subscription.ClientCalls.erase(dataChange) && subscription.ClientCalls.empty() )
				toDelete.try_emplace( h.SubId() ).first->second.emplace( h.MonitorId() );
		}
		if( toDelete.size() )
			DeleteMonitoring( _pClient->UAPointer(), toDelete );
	}

	α UAMonitoringNodes::Unsubscribe( flat_set<NodeId>&& nodes, sp<IDataChange> dataChange )ι->tuple<flat_set<NodeId>,flat_set<NodeId>>{
		flat_map<SubscriptionId,flat_set<MonitorId>> toDelete;
		tuple<flat_set<NodeId>,flat_set<NodeId>> successFailures;
		ul _{ _mutex };
		for( auto& node : nodes ){
			if( auto [id,pSubscription] = FindNode(node); pSubscription && pSubscription->ClientCalls.erase(dataChange) ){
				get<0>(successFailures).emplace( move(node) );
				if( pSubscription->ClientCalls.empty() )
					toDelete.try_emplace( id.SubId() ).first->second.emplace( id.MonitorId() );
			}
			else{
				TRACE( "Could not find node '{}' for unsubscription.", node.to_string() );
				get<1>(successFailures).emplace( move(node) );
			}
		}
		if( toDelete.size() )
			DeleteMonitoring( _pClient->UAPointer(), toDelete );
		return successFailures;
	}

	α UAMonitoringNodes::DeleteMonitoring( UA_Client* ua, flat_map<SubscriptionId,flat_set<MonitorId>> requested )ι->Task{
		auto wait = 5s;
		TRACE( "[{:x}]DeleteMonitoring count={}, wait={}", (uint)ua, requested.size(), Chrono::ToString(wait) ); //duration_cast<std::chrono::seconds>(wait).count()
		co_await Threading::Alarm::Wait( wait );
		auto pClient = UAClient::TryFind(ua); if( !pClient ) co_return;

		flat_map<UA_UInt32,flat_set<MonitorId>> toDelete;
		ul lock{ _mutex };
		for( auto& [subscriptionId, monitoredIds] : requested ){
			for( auto& monitoredId : monitoredIds ){
				const MonitorHandle h{subscriptionId,monitoredId};
				if( auto p = _subscriptions.find(h); p!=_subscriptions.end() && p->second.ClientCalls.empty() ){
					TRACE( "[{:x}.{:x}]DeleteMonitoring for:  {}", _pClient->Handle(), (Handle)h, p->second.Node.to_string() );
					_subscriptions.erase( h );
					toDelete.try_emplace( subscriptionId ).first->second.emplace( monitoredId );
				}
			}
		}
		if( _subscriptions.size()==0 )
			_pClient->StopProcessDataSubscriptions();
		lock.unlock();
		for( auto& [subscriptionId, monitorIds] : toDelete )
			pClient->DataSubscriptionDelete( subscriptionId, move(monitorIds) );
	}
}
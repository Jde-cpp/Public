#pragma once
#include <jde/opc/uatypes/Node.h>
#include <jde/framework/coroutine/Timer.h>
#include <jde/framework/math/HiLow.h>
#include "../uatypes/MonitoredItemCreateResult.h"
#include "../async/DataChanges.h"
//#include "../../../../../Framework/source/coroutine/Awaitable.h"

namespace Jde::Opc{ struct Value; }
namespace Jde::Opc::Gateway{
	/*struct SocketSession;*/ struct UAClient;

	struct IDataChange{
		β SendDataChange( const OpcClientNK& opcId, const ExNodeId& node, const Value& value )ι->void=0;
		β to_string()Ι->string=0;
	};

	struct MonitorHandle final : HiLow{
		MonitorHandle( SubscriptionId s, MonitorId m )ι:HiLow{ s, m }{}
		MonitorHandle( Handle x )ι:HiLow{ x }{}
		α SubId()Ι->SubscriptionId{ return Hi(); }
		α MonitorId()Ι->MonitorId{ return Low(); }
	};


	struct UAMonitoringNodes final{
		UAMonitoringNodes(UAClient* p)ι:_pClient{p}{}
		~UAMonitoringNodes(){_pClient = nullptr;}
		α Shutdown()ι->void;
		α Subscribe( sp<IDataChange>&& dataChange, flat_set<ExNodeId>&& nodes, DataChangeAwait::Handle h, Handle& requestId )ι->void;
		α Unsubscribe( flat_set<ExNodeId>&& nodes, sp<IDataChange> dataChange )ι->tuple<flat_set<ExNodeId>,flat_set<ExNodeId>>;
		α Unsubscribe( sp<IDataChange> )ι->void;
		α SendDataChange( Handle h, const Value&& value )ι->uint;
		α OnCreateResponse( UA_CreateMonitoredItemsResponse* response, Handle requestId )ι->void;
		α GetResult( Handle requestId, StatusCode status )ι->FromServer::SubscriptionAck;
	private:
		struct Subscription{
			Subscription( /*OpcClientNK opcId,*/ ExNodeId node, MonitoredItemCreateResult result, sp<IDataChange> clientCall )ι: /*OpcClientNK{move(opcId)},*/ Node{ move(node) }, Result{ move(result) }, ClientCalls{ move(clientCall) }{}
			ExNodeId Node;
			MonitoredItemCreateResult Result;
			flat_set<sp<IDataChange>> ClientCalls;
		};
		α GetClient()ι->sp<UAClient>;
		α FindNode( const ExNodeId& node )ι->tuple<MonitorHandle,Subscription*>;
		α DeleteMonitoring( UA_Client* ua, flat_map<SubscriptionId,flat_set<MonitorId>> requested )ι->DurationTimer::Task;

		atomic<RequestId> _requestId{};
		flat_map<MonitorHandle,flat_set<ExNodeId>> _requests;
		flat_map<MonitorHandle,tuple<flat_set<ExNodeId>,sp<IDataChange>>> _calls;
		flat_map<MonitorHandle,flat_map<ExNodeId,StatusCode>> _errors;
		shared_mutex _mutex;
		flat_map<MonitorHandle,Subscription> _subscriptions;
		UAClient* _pClient;
	};
}
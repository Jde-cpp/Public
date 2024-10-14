#pragma once

namespace Jde::Iot{
	struct CreateMonitoredItemsRequest : UA_CreateMonitoredItemsRequest{
		CreateMonitoredItemsRequest( flat_set<NodeId>&& nodes )ι;
		CreateMonitoredItemsRequest( CreateMonitoredItemsRequest&& x )ι;
		~CreateMonitoredItemsRequest(){ UA_CreateMonitoredItemsRequest_clear( this ); }
		α operator=( CreateMonitoredItemsRequest&& )ι->UA_CreateMonitoredItemsRequest;
		α ToJson()ι->json;
	};

	inline CreateMonitoredItemsRequest::CreateMonitoredItemsRequest( flat_set<NodeId>&& nodes )ι{
		UA_CreateMonitoredItemsRequest_init( this );
		itemsToCreateSize = nodes.size();
		itemsToCreate = (UA_MonitoredItemCreateRequest*)UA_Array_new( itemsToCreateSize, &UA_TYPES[UA_TYPES_MONITOREDITEMCREATEREQUEST] );
		uint i=0;
		for( auto& n : nodes ){
			auto& item = itemsToCreate[i++];
			item.itemToMonitor.nodeId = n.Move();
			item.itemToMonitor.attributeId = UA_ATTRIBUTEID_VALUE;
			item.monitoringMode = UA_MONITORINGMODE_REPORTING;
			item.requestedParameters.samplingInterval = 500.0;
			item.requestedParameters.queueSize = 1;
			item.requestedParameters.discardOldest = true;
		}
	}
	inline CreateMonitoredItemsRequest::CreateMonitoredItemsRequest( CreateMonitoredItemsRequest&& x )ι:
		UA_CreateMonitoredItemsRequest{ x }{
		UA_CreateMonitoredItemsRequest_init( &x );
	}

	Ξ CreateMonitoredItemsRequest::operator=( CreateMonitoredItemsRequest&& )ι->UA_CreateMonitoredItemsRequest{
		UA_CreateMonitoredItemsRequest r = *this;
		UA_CreateMonitoredItemsRequest_init( this );
		return r;
	}

	Ξ CreateMonitoredItemsRequest::ToJson()ι->json{
		json values = json::array();
		for( auto request : Iterable<UA_MonitoredItemCreateRequest>(itemsToCreate, itemsToCreateSize) ){
			json j;
			j["nodeId"] = Iot::ToJson( request.itemToMonitor.nodeId );
			j["attributeId"] = request.itemToMonitor.attributeId==13 ? "VALUE" : std::to_string(request.itemToMonitor.attributeId);
			j["monitoringMode"] = request.monitoringMode;
			//j["samplingInterval"] = item.requestedParameters.samplingInterval;
			//j["itemsToCreate"][i]["requestedParameters"]["queueSize"] = item.requestedParameters.queueSize;
			//j["itemsToCreate"][i]["requestedParameters"]["discardOldest"] = item.requestedParameters.discardOldest;
			values.push_back( j );
		}
		return values;	
	}

}
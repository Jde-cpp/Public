#pragma once

namespace Jde::Iot
{
	struct CreateMonitoredItemRequest : UA_MonitoredItemCreateRequest{
		CreateMonitoredItemRequest( NodeId nodeId )ι:
			itemToMonitor{ nodeId, UA_ATTRIBUTEID_VALUE, UA_String{}, UA_QualifiedName{} },
	    monitoringMode{ UA_MONITORINGMODE_REPORTING },
			requestedParameters{ {}, 250, UA_ExtensionObject{}, 1, true }
		{}
	};
}
﻿#pragma once

namespace Jde::Opc{

	struct MonitoredItemCreateResult : UA_MonitoredItemCreateResult{
		MonitoredItemCreateResult()ι{ UA_MonitoredItemCreateResult_init(this); }
		MonitoredItemCreateResult( UA_UInt32 id )ι:MonitoredItemCreateResult{}{ UA_MonitoredItemCreateResult_init(this); monitoredItemId=id; }
		MonitoredItemCreateResult( const UA_MonitoredItemCreateResult& x )ι{UA_MonitoredItemCreateResult_copy( &x, this );}
		MonitoredItemCreateResult( const MonitoredItemCreateResult& x )ι:MonitoredItemCreateResult{(UA_MonitoredItemCreateResult&)x}{}
		MonitoredItemCreateResult( MonitoredItemCreateResult&& x )ι:MonitoredItemCreateResult{x}{ UA_MonitoredItemCreateResult_init(&x); }
		~MonitoredItemCreateResult(){ UA_MonitoredItemCreateResult_clear(this);}
		α operator=( MonitoredItemCreateResult&& x )ι->MonitoredItemCreateResult&{ UA_MonitoredItemCreateResult_copy( &x, this ); return *this; }
		α operator=( const MonitoredItemCreateResult& x )ι->MonitoredItemCreateResult&{ UA_MonitoredItemCreateResult_copy( &x, this ); return *this; }
	};

	Ξ operator==( const MonitoredItemCreateResult& x, const MonitoredItemCreateResult& y )ι->bool{ return x.monitoredItemId==y.monitoredItemId; }
}
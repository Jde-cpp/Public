#pragma once

namespace Jde::Opc::Gateway{
	struct MonitoredItemCreateResult : UA_MonitoredItemCreateResult{
		MonitoredItemCreateResult()ι{ UA_MonitoredItemCreateResult_init(this); }
		MonitoredItemCreateResult( UA_UInt32 id )ι:MonitoredItemCreateResult{}{ UA_MonitoredItemCreateResult_init(this); monitoredItemId=id; }
		MonitoredItemCreateResult( const UA_MonitoredItemCreateResult& x )ι{UA_MonitoredItemCreateResult_copy( &x, this );}
		MonitoredItemCreateResult( const MonitoredItemCreateResult& x )ι:MonitoredItemCreateResult{(UA_MonitoredItemCreateResult&)x}{}
		MonitoredItemCreateResult( MonitoredItemCreateResult&& x )ι{ *(UA_MonitoredItemCreateResult*)this = x; UA_MonitoredItemCreateResult_init(&x); }//shallow-transfer ownership, then zero the source so its dtor won't free what we now own (a deep copy here would leak the source's allocations).
		~MonitoredItemCreateResult(){ UA_MonitoredItemCreateResult_clear(this);}
		α operator=( MonitoredItemCreateResult&& x )ι->MonitoredItemCreateResult&{ if( this!=&x ){ UA_MonitoredItemCreateResult_clear(this); *(UA_MonitoredItemCreateResult*)this = x; UA_MonitoredItemCreateResult_init(&x); } return *this; }//clear our old contents first, then shallow-transfer + zero source.
		α operator=( const MonitoredItemCreateResult& x )ι->MonitoredItemCreateResult&{ if( this!=&x ){ UA_MonitoredItemCreateResult_clear(this); UA_MonitoredItemCreateResult_copy( &x, this ); } return *this; }//UA_copy zeroes the destination, so clear our old allocations first to avoid leaking them.
	};

	Ξ operator==( const MonitoredItemCreateResult& x, const MonitoredItemCreateResult& y )ι->bool{ return x.monitoredItemId==y.monitoredItemId; }
}
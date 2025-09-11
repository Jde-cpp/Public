#include <jde/opc/uatypes/UAString.h>

namespace Jde::Opc{
	UAString::UAString( uint size )ι:
		UA_String{ size, size ? (UA_Byte*)UA_malloc( size ) : nullptr }
	{}
	UAString::~UAString()ι{
		UA_String_clear( this );
	}
}
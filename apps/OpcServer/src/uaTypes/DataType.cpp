#include "DataType.h"
#include "../UAServer.h"

namespace Jde::Opc {
	α Server::DT( NodePK nodePK, SL sl )ε->const UA_DataType&{
		return GetUAServer().GetDataType( nodePK, sl );
	}
	α Server::DT( const jvalue& j, SL sl )ε->const UA_DataType&{
		if( j.is_number() )
			return DT( j.to_number<uint32>(), sl );

		THROW_IFSL( !j.is_string(), "Unsupported data type: '{}'", serialize(j) );
		let& type = j.get_string();
		if( type=="bool" )
			return UA_TYPES[UA_TYPES_BOOLEAN];
		if( type=="string" )
			return UA_TYPES[UA_TYPES_STRING];
		if( type=="double" )
			return UA_TYPES[UA_TYPES_DOUBLE];
		if( type=="int" )
			return UA_TYPES[UA_TYPES_INT32];
		if( type=="uint" )
			return UA_TYPES[UA_TYPES_UINT32];
		if( type=="date" )
			return UA_TYPES[UA_TYPES_DATETIME];
		if( type=="guid" )
			return UA_TYPES[UA_TYPES_GUID];
		if( type=="byte" )
			return UA_TYPES[UA_TYPES_BYTE];
		if( type=="bytes" )
			return UA_TYPES[UA_TYPES_BYTESTRING];
		if( type=="xml" )
			return UA_TYPES[UA_TYPES_XMLELEMENT];
		if( type=="variant" )
			return UA_TYPES[UA_TYPES_VARIANT];
		if( type=="status" )
			return UA_TYPES[UA_TYPES_STATUSCODE];
		if( type=="nodeid" )
			return UA_TYPES[UA_TYPES_NODEID];
		if( type=="qualifiedname" )
			return UA_TYPES[UA_TYPES_QUALIFIEDNAME];
		if( type=="localizedtext" )
			return UA_TYPES[UA_TYPES_LOCALIZEDTEXT];
		if( type=="extensionobject" )
			return UA_TYPES[UA_TYPES_EXTENSIONOBJECT];
		if( type=="dataValue" )
			return UA_TYPES[UA_TYPES_DATAVALUE];
		if( type=="structure" )
			return UA_TYPES[UA_TYPES_STRUCTURETYPE];
		if( type=="enumeration" )
			return UA_TYPES[UA_TYPES_ENUMERATION];
		THROW( "Unsupported data type: '{}'", type );
	}
}
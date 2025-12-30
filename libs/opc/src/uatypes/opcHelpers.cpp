#include <jde/opc/uatypes/opcHelpers.h>
#include <jde/opc/uatypes/NodeId.h>

namespace Jde{
	α Opc::FindDataType( const NodeId& nodeId )ι->UA_DataType*{
		for( uint i=0; i<UA_TYPES_COUNT; ++i ){
			if( NodeId{UA_TYPES[i].typeId}==nodeId )
				return &UA_TYPES[i];
		}
		return nullptr;
	}
}

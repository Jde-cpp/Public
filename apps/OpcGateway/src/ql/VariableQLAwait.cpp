#include "VariableQLAwait.h"
#include <jde/fwk/exceptions/ArgException.h>
#include "../async/ConnectAwait.h"
#include <jde/opc/uatypes/Variant.h>

#define let const auto

namespace Jde::Opc::Gateway{
	α VariableQLAwait::Execute()ι->TAwait<sp<UAClient>>::Task{
		auto opcId = _mutation.FindPtr<jstring>( "opc" );
		try{
			_client = co_await ConnectAwait{ string{opcId ? *opcId : sv{}}, _session->SessionId, _session->UserPK, _sl };
			_nodeId = NodeId{ _mutation.As<>("id") };
			if( _mutation.Type==QL::EMutationQL::Update )
				ReadDataType( _mutation.As<>("value"), _nodeId );
			else
				ResumeExp( Argε("Only update is supported") );

		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α VariableQLAwait::ReadDataType( jvalue value, const NodeId& nodeId )ι->TAwait<ReadResponse>::Task{
		try{
			const UA_DataType* dt = nullptr;
			auto dtId = co_await ReadAwait{ nodeId, UA_ATTRIBUTEID_DATATYPE, _client };
			auto typeNodeId = dtId.ScalerNodeId();
			if( !typeNodeId.namespaceIndex )
				dt = FindDataType( typeNodeId );
			else{
				//see if we can get type from value
				auto v = ( co_await ReadAwait{ nodeId, UA_ATTRIBUTEID_VALUE, _client } ).ScalerValue();
				dt = v ? v->type : nullptr;
			}

			Write( Value{move(value), dt} );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α VariableQLAwait::Write( Value&& value )ι->TAwait<WriteResponse>::Task{
		try{
			auto response = co_await WriteAwait{ _nodeId, move(value), _client };
			if( _mutation.ResultRequest.has_value() )
				Read( move(*_mutation.ResultRequest) );
			else
				Resume( jvalue{} );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α VariableQLAwait::Read( QL::TableQL&& ql )ι->TAwait<ReadResponse>::Task{
		try{
			auto response = co_await ReadAwait{ {{_nodeId}, move(ql)}, _client };
			Resume( response.ToJson(ql) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}
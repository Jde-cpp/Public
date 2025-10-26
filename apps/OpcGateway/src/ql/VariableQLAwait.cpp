#include "VariableQLAwait.h"
#include <jde/fwk/exceptions/ArgException.h>
#include "../async/ConnectAwait.h"


namespace Jde::Opc::Gateway{
	α VariableQLAwait::Execute()ι->TAwait<sp<UAClient>>::Task{
		auto opcId = _mutation.FindPtr<jstring>( "opc" );
		try{
			_client = co_await ConnectAwait{ string{opcId ? *opcId : sv{}}, _session->SessionId, _session->UserPK, _sl };
			_nodeId = NodeId{ _mutation.As<>("id") };
			if( _mutation.Type==QL::EMutationQL::Update )
				ReadDataType( _mutation.As<>("value") );
			else
				ResumeExp( Argε("Only update is supported") );

		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α VariableQLAwait::ReadDataType( jvalue value )ι->TAwait<ReadResponse>::Task{
		try{
			auto type = ( co_await ReadAwait{_nodeId, UA_ATTRIBUTEID_DATATYPE, _client} ).ScalerDataType();
			Write( Value{move(value), type} );
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
			Resume( (co_await ReadAwait{{_nodeId, move(ql)}, _client}).ScalerJson() );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}
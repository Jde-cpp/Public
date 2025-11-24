#include "CallAwait.h"
#include <jde/opc/uatypes/Variant.h>
#include <jde/fwk/utils/collections.h>

namespace Jde::Opc::Gateway{
	CallResponse::CallResponse( CallResponse&& x )ι:
		UA_CallResponse{ x }{
		UA_CallResponse_init( &x );
	}

	CallResponse::CallResponse( UA_CallResponse&& x )ι:
		UA_CallResponse{ x }{
		UA_CallResponse_init( &x );
	}
	α CallResponse::operator=( CallResponse&& x )ι->CallResponse&{
		if( this != &x ){
			UA_CallResponse_clear( this );
			*(UA_CallResponse*)this = (UA_CallResponse&)x;
			UA_CallResponse_init( &x );
		}
		return *this;
	}

	α CallResponse::Validate( Handle uahandle, RequestId requestId, SL sl )ε->void{
		if( responseHeader.serviceResult )
			throw UAClientException{ responseHeader.serviceResult, uahandle, requestId, sl };
	}
	α CallResponse::ToJson()ι->jvalue{
		return jobject{};
/*		jarray resultsJson;
		for( size_t i=0; i<resultsSize; ++i )
			resultsJson.push_back( results[i].ToJson() );
		jarray diagnosticInfosJson;
		for( size_t i=0; i<diagnosticInfosSize; ++i )
			diagnosticInfosJson.push_back( diagnosticInfos[i].ToJson() );
		return jobject{
			{"responseHeader", responseHeader.ToJson()},
			{"results", resultsJson},
			{"diagnosticInfos", diagnosticInfosJson}
		};*/
	}

	Ω callback( UA_Client*, void* hptr, UA_UInt32, UA_CallResponse* cr )ι->void{
		auto& h = *(CallAwait::Handle*)hptr;
		h.promise().Resume( CallResponse{move(*cr)}, h );
	}
	α CallAwait::Execute()ι->TAwait<sp<UAClient>>::Task{
		try{
			_client = co_await ConnectAwait{ Json::FindString(_ql.Args,"opc").value_or(""), *_session, _sl };

			jarray jargs = Json::FindDefaultArray( _ql.Args, "args" );
			auto args = Reserve<Variant>( jargs.size() );
			// for( size_t i=0; i<jargs.size(); ++i )
			// 	args[i] = Variant{ jargs[i], &UA_TYPES[UA_TYPES_VARIANT] };
			UA_Client_call_async(
				*_client,
				NodeId{ _ql.Args },
				NodeId{ _ql.As<jobject>("method") },
				jargs.size(),
				args.data(),
				callback,
				&_h,
				&_requestId
			);
			_client->Process( _requestId, "call" );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α CallAwait::await_resume()ι->CallResponse{
		base::CheckException();
		_client->ClearRequest( _requestId );
		CallResponse y{ std::move(*Promise()->Value()) };
		y.Validate( _client->Handle(), _requestId, _sl );
		return y;
	}

	α JCallAwait::Execute()ι->TAwait<CallResponse>::Task{
		try{
			auto response = co_await CallAwait{ move(_ql), move(_session), _sl };
			Resume( response.ToJson() );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}
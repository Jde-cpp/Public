#pragma once
//#include <google/protobuf/message_lite.h>
//#include <jde/http/usings.h>
//#include <jde/http/exports.h>
//#include "../../../../Framework/source/io/ProtoUtilities.h"
#include <jde/http/IClientSocketSession.h>

namespace Jde::Http{
	//TODO - derive from TAwait
	template<class T>
	struct ClientSocketAwait final{
		using Task = TTask<T>;
		using PromiseType = Task::promise_type;
		using Handle = coroutine_handle<PromiseType>;
		ClientSocketAwait( sp<IClientSocketSession> session, RequestId requestId, string&& request, SRCE )ι;
		α await_ready()ι->bool{ return false; }
		α await_suspend( Handle h )ι->void;
		α await_resume()ε->T;
	private:
		PromiseType* _promise{};
		string _request;
		RequestId _requestId;
		sp<IClientSocketSession> _session;
		SL _sl;
	};

	Τ ClientSocketAwait<T>::ClientSocketAwait( sp<IClientSocketSession> session, RequestId requestId, string&& request, SL sl )ι:
		_request{ move(request) },
		_requestId{ requestId },
		_session{ session },
		_sl{ sl }
	{}

	Ŧ ClientSocketAwait<T>::await_suspend( coroutine_handle<PromiseType> h )ι->void{
		_session->AddTask( _requestId, h );
		_promise = &h.promise();
		_session->Write( move(_request) );
	}

	Ŧ ClientSocketAwait<T>::await_resume()ε->T{
		ASSERT( _promise && (_promise->Result || _promise->Exception) );
		if( _promise->Exception )
			_promise->Exception->Throw();
		return move( *_promise->Result );
	}
}


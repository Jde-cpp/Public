#pragma once
#include <jde/http/usings.h>
#include <jde/http/exports.h>
#include <jde/http
namespace Jde::Http{
	struct ClientSocketAwait;
	struct ClientSocketTask{
		struct promise_type{
			promise_type()ι{}
			α get_return_object()ι->ClientSocketTask{ return {}; }
			suspend_never initial_suspend()ι{ return {}; }
			suspend_never final_suspend()ι{ return {}; }
			α return_void()ι->void{}
			Φ unhandled_exception()ι->void;
			up<google::protobuf::MessageLite> _result;
		};
	};
	using HClientSocketTask = coroutine_handle<ClientSocketTask::promise_type>;
	struct ClientSocketAwait{
		using Session = IClientSocketSession;
		ClientSocketAwait( sp<Session> session, google::protobuf::MessageLite& request, SRCE )ι:_session{session}, _sl{sl}{}
		α await_ready()ι->bool{ return false; }
		α await_suspend( HClientSocketTask h )ι->void;
		α await_resume()ε->HttpTaskResult;
	protected:
		sp<IClientSocketSession> _session;
		SL _sl;
	};

	template<class TRequest, class TResult>
	α ClientSocketAwait::await_suspend( HClientSocketTask h )ι->void{
		h.promise().Awaitable = this;
		_session->WriteRequestId( move(_input) );
	}
*/
}
#undef Φ
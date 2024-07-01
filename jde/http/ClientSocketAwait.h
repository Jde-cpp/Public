#pragma once
#include <jde/http/usings.h>
#include <jde/http/exports.h>

#define Φ ΓH auto
namespace Jde::Http{

/*	template<class TFromServerMsgs>
	struct CheckRequestId{
		β IsResponse( TFromServerMsgs& t )ι->optional<TFromServerMsgs>{ return nullopt; }
	};

	template<class T>
	struct ClientSocketResult{
		ClientSocketResult()=default;
		ClientSocketResult( ClientSocketResult&& req )ι:Request{move(req)}{}
		ClientSocketResult( ClientSocketResult&& rhs )ι;
		ClientSocketResult( ClientSocketResult&& req, T&& res )ι:Request{ move(req) }, Result( move(res) ){}
		α operator=( ClientSocketResult&& rhs)ι->ClientSocketResult&;

		optional<T> Result;
	};
	*/
/*
	struct ClientSocketAwait;
	struct ClientSocketTask{
		struct promise_type{
			promise_type()ι{}
			α get_return_object()ι->ClientSocketTask{ return {}; }
			suspend_never initial_suspend()ι{ return {}; }
			suspend_never final_suspend()ι{ return {}; }
			α return_void()ι->void{}
			Φ unhandled_exception()ι->void;

			ClientSocketAwait* Awaitable{};
		};
	};
	using HClientSocketTask = coroutine_handle<ClientSocketTask::promise_type>;

	template<class TFromClientMsgs, class TFromServerMsgs>
	struct ClientSocketAwait{
		using Session = TClientSocketSession<TFromServerMsgs, TFromServerMsg, TFromClientMsgs, TFromClientMsg>;
		ClientSocketAwait( sp<Session> session, ICheckRequestId<TFromServerMsgs>&& check, SRCE )ι:_session{session}, _check{ move(check) }, _sl{sl}{}
		virtual ~ClientSocketAwait()=0;
		α await_ready()ι->bool{ return false; }
		α await_suspend( HClientSocketTask h )ι->void;
		α await_resume()ε->HttpTaskResult;
	protected:
		sp<Session> _session;
		ICheckRequestId<TFromServerMsgs> _check;
		optional<TResult> _result;
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
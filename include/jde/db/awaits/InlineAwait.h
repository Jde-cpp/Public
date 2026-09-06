#pragma once
#include <jde/fwk/co/Await.h>

namespace Jde::DB{
	//An awaitable that may finish inside await_ready.  An in-process driver (sqlite) has no socket to wait for - the
	//statement runs before the coroutine would suspend - and saying so with await_ready==true, rather than suspending and
	//resuming inline, keeps the continuation off the caller's frame: Await.h's Resume() is a plain h.resume() with no
	//symmetric transfer, so N back-to-back inline resumes nested N deep and only unwound when the chain finally suspended
	//for real or finished.  A coroutine that never suspends has no handle, so the promise cannot carry the result or the
	//error; Complete parks them here and await_resume answers from here.  When await_ready said false the base's
	//promise-backed await_resume runs as usual.
	//TBase is the TAwait/TAwaitEx the concrete awaitable would otherwise derive from - the same Task, so the pairing
	//rule (an awaitable dictates its caller's return type) is unchanged.
	template<class TResult, class TBase=TAwait<TResult>>
	struct InlineAwait : TBase{
		using TBase::TBase;
		α await_resume()ε->TResult override{
			if( _exception )
				_exception->Throw();
			return _inlined ? move(_result) : TBase::await_resume();
		}
	protected:
		//Runs `run` now, on the caller's thread, and keeps what it produced - the value or the exception - for
		//await_resume.  Always answers true so an await_ready override can `return <ready-test> && Complete(…)`.
		α Complete( auto&& run )ι->bool{
			_inlined = true;
			try{
				_result = run();
			}
			catch( Exception& e ){ //virtual Move() keeps the driver's exception type - await_resume rethrows the dynamic type.
				_exception = e.Move();
			}
			catch( runtime_error& e ){
				_exception = mu<Exception>( move(e) );
			}
			return true;
		}
	private:
		TResult _result{};
		up<Exception> _exception;
		bool _inlined{};
	};
}
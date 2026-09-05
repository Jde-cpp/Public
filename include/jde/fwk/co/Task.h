#pragma once
#include <exception>
#ifndef TASK_H
#define TASK_H
#include "../log/break.h"

namespace Jde{
	template<class Task,class TResult>
	struct IPromise{
		α get_return_object()ι->Task{ return {}; }
		suspend_never initial_suspend()ι{ return {}; }
		suspend_never final_suspend()noexcept{ return {}; }
		α return_void()ι->void{}
		α unhandled_exception()ι->void;
		β Exp()Ι->const up<Exception>& =0;
		β MoveExp()ι->up<Exception> =0;
		β SetExp( Exception&& x )ι->void=0;
		α SetExp( std::runtime_error&& x )ι->void{
			if( auto p = dynamic_cast<Exception*>(&x); p )
				SetExp( move(*p) );
			else if( auto p = dynamic_cast<runtime_error*>(&x); p )
				SetExp( Exception{move(*p)} );
		}
		α ResumeExp( Exception&& e, coroutine_handle<> h )ι->void{ SetExp( move(e) ); h.resume(); };
		α ResumeExp( std::runtime_error&& e, coroutine_handle<> h )ι->void{ SetExp( move(e) ); h.resume(); };
	protected:
		TResult Expected;
	};

	template<class Task,class TResult,class TError=up<Exception>>
	struct IExpectedPromise : IPromise<Task,variant<up<Exception>,TResult>>{
		using base = IPromise<Task,variant<up<Exception>,TResult>>;
		using TExpected=variant<TError,TResult>;
		α Exp()Ι->const up<Exception>& override{ return base::Expected.index()==0 ? std::get<0>(base::Expected) : Exception::EmptyPtr(); }
		α MoveExp()ι->up<Exception> override{ return base::Expected.index()==0 ? move(std::get<0>(base::Expected)) : up<Exception>{}; }
		α Emplaced()ι->bool{ return base::Expected.index()==1 || (base::Expected.index()==0 && Exp()!=nullptr); }
		α Value()ι->TResult*{ return base::Expected.index()==1 ? &std::get<1>(base::Expected) : nullptr; }
		α SetValue( TResult&& x )ι->void{ base::Expected = std::move(x); }
//		α SetScaler( TResult x )ι->void{ base::Expected = x; } windows doesn't work.
		α Resume( TResult&& x, coroutine_handle<> h )ι->void{ SetValue(std::move(x)); h.resume(); };
//		α ResumeScaler( TResult x, coroutine_handle<> h )ι->void{ SetScaler(x); h.resume(); }; windows doesn't work.
		using base::SetExp;//the override below would otherwise hide the base's std::exception overload.
		α SetExp( Exception&& e )ι->void override{ base::Expected = e.Move(); }
	};
	template<class Task>
	struct VoidPromise : IPromise<Task,up<Exception>>{
		using base = IPromise<Task,up<Exception>>;
		α Exp()Ι->const up<Exception>& override{ return base::Expected; }
		α MoveExp()ι->up<Exception> override{ return move(base::Expected); }
		using base::SetExp;
		α SetExp( Exception&& x )ι->void override{ base::Expected = x.Move(); }
	};
	struct VoidTask{
		struct promise_type : VoidPromise<VoidTask>{};
	};

	template<class TResult>
	struct TTask final{
		struct promise_type : IExpectedPromise<TTask<TResult>, TResult, up<Exception>>
		{};
	};

	template<class Task,class TExpected>
	α IPromise<Task,TExpected>::unhandled_exception()ι->void{
		try{
			BREAK;
			throw;
		}
		catch( Exception& e ){
			e.SetLevel( ELogLevel::Critical );
			SetExp( move(e) );
		}
		catch( runtime_error& e ){
			SetExp( Exception{SRCE_CUR, {ELogLevel::Critical}, move(e), "runtime_error - {}", e.what()} );
		}
		catch( ... ){
			SetExp( Exception{"unknown exception", {ELogLevel::Critical}} );
		}
	}
}
#endif
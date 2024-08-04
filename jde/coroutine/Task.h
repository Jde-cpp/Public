#pragma once
#ifndef TASK_H
#define TASK_H

namespace Jde{
	template<class TTask,class TResult>
	struct IPromise{
		α get_return_object()ι->TTask{ return {}; }
		suspend_never initial_suspend()ι{ return {}; }
		suspend_never final_suspend()ι{ return {}; }
		α return_void()ι->void{}
		α unhandled_exception()ι->void;
		β Error()Ι->const up<IException>& =0;
		β MoveError()ι->up<IException> =0;
		β SetError( IException&& x )ι->void =0;
		α ResumeWithError( IException&& e, coroutine_handle<> h )ι->void{ SetError( move(e) ); h.resume(); };
	protected:
		TResult Expected;
	};

	template<class TTask,class TResult,class TError=up<IException>>
	struct IExpectedPromise : IPromise<TTask,variant<up<IException>,TResult>>{
		using base = IPromise<TTask,variant<up<IException>,TResult>>;
		using TExpected=variant<TError,TResult>;
		α Error()Ι->const up<IException>& override{ return base::Expected.index()==0 ? std::get<0>(base::Expected) : IException::EmptyPtr(); }
		α MoveError()ι->up<IException> override{ return base::Expected.index()==0 ? move(std::get<0>(base::Expected)) : up<IException>{}; }
		α SetError( IException&& e )ι->void override{ base::Expected = e.Move(); }
		α Emplaced()ι->bool{ return base::Expected.index()==1 || (base::Expected.index()==0 && Error()!=nullptr); }
		α Value()ι->TResult*{ return base::Expected.index()==1 ? &std::get<1>(base::Expected) : nullptr; }
		α SetValue( TResult&& x )ι->void{ base::Expected = move(x); }
		α Resume( TResult&& x, coroutine_handle<> h )ι->void{ SetValue(move(x)); h.resume(); };
	};
	template<class TTask>
	struct VoidPromise : IPromise<TTask,up<IException>>{
		using base = IPromise<TTask,up<IException>>;
		α Error()Ι->const up<IException>& override{ return base::Expected; }
		α MoveError()ι->up<IException> override{ return move(base::Expected); }
		α SetError( IException&& x )ι->void override{ base::Expected = x.Move(); }
	};
	struct VoidTask{
		struct promise_type : VoidPromise<VoidTask>{};
	};

	template<class TResult>
	struct TTask final{
		using Expected=variant<up<IException>,TResult>;
		struct promise_type : IExpectedPromise<TTask<TResult>, TResult, up<IException>>
		{};
	};

	template<class TTask,class TExpected>
	α IPromise<TTask,TExpected>::unhandled_exception()ι->void{
		try{
			BREAK;
			throw;
		}
		catch( IException& e ){
			e.SetLevel( ELogLevel::Critical );
			SetError( move(e) );
		}
		catch( nlohmann::json::exception& e ){
			SetError( Jde::Exception{SRCE_CUR, move(e), ELogLevel::Critical, "json exception - {}", e.what()} );
		}
		catch( std::exception& e ){
			SetError( Jde::Exception{SRCE_CUR, move(e), ELogLevel::Critical, "std::exception - {}", e.what()} );
		}
		catch( ... ){
			SetError( Jde::Exception{SRCE_CUR, ELogLevel::Critical, "unknown exception"} );
		}
	}
}
#endif
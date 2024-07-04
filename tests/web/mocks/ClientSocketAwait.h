#pragma once
#include <google/protobuf/message_lite.h>
#include <jde/http/usings.h>
#include <jde/http/exports.h>
#include "../../../../Framework/source/io/ProtoUtilities.h"
//#include "ClientSocketSessionMock.h"
#include <jde/http/IClientSocketSession.h>

namespace Jde::Http{
	struct IClientSocketSession;
/*
	template<class T>
	struct TTask{
		struct promise_type{
			promise_type()ι{}
			α get_return_object()ι->TTask<T>{ return {}; }
			suspend_never initial_suspend()ι{ return {}; }
			suspend_never final_suspend()ι{ return {}; }
			α return_void()ι->void{}
			α unhandled_exception()ι->void;
			T Result;
		};
	};

	using ClientSocketMessageTask = TTask<up<google::protobuf::MessageLite>>;
	using HClientSocketMessageTask = coroutine_handle<ClientSocketMessageTask::promise_type>;
	struct ClientSocketMessageAwait final{
		ClientSocketMessageAwait( sp<IClientSocketSession> session, RequestId requestId, string&& request, SRCE )ι;
		α await_ready()ι->bool{ return false; }
		α await_suspend( HClientSocketMessageTask h )ι->void;
		α await_resume()ε->up<google::protobuf::MessageLite>;
	protected:
		ClientSocketMessageTask::promise_type* _promise{};
		string _request;
		RequestId _requestId;
		sp<IClientSocketSession> _session;
		SL _sl;
	};
*/
	template<class T>
	struct ClientSocketAwait final{
		using Task = TTask<T>;
		using PromiseType = Task::promise_type;
		using Handle = coroutine_handle<PromiseType>;
		ClientSocketAwait( sp<IClientSocketSession> session, RequestId requestId, string&& request, SRCE )ι;
		α await_ready()ι->bool{ return false; }
		//α Await( coroutine_handle<PromiseType> h )ι->ClientSocketMessageTask;
		α await_suspend( Handle h )ι->void;
		α await_resume()ε->T;
	private:
		PromiseType* _promise{};
		//ClientSocketMessageAwait _await;
		//ClientSocketMessageTask::promise_type* _promise{};
		string _request;
		RequestId _requestId;
		sp<IClientSocketSession> _session;
		SL _sl;
	};

	Τ using HttpTask = ClientSocketAwait<T>::Task;

	Τ ClientSocketAwait<T>::ClientSocketAwait( sp<IClientSocketSession> session, RequestId requestId, string&& request, SL sl )ι:
		//_await{ session, requestId, move(request), sl }
		_request{ move(request) },
		_requestId{ requestId },
		_session{ session },
		_sl{ sl }
	{}

	Ŧ ClientSocketAwait<T>::await_suspend( coroutine_handle<PromiseType> h )ι->void{
		//Await( h );
		_session->AddTask( _requestId, h );
		//_session->AddTask( _requestId, h );
		//_tasks.emplace( _requestId, h );
		_promise = &h.promise();
		_session->Write( move(_request) );
	}

//	Ŧ ClientSocketAwait<T>::Await( coroutine_handle<PromiseType> h )ι->ClientSocketMessageTask{
		//auto message = co_await _await;
		// auto result = dynamic_cast<T*>( message.get() );
		// ASSERT( result );
		// h.promise().Result = move( *result );
		// h.resume();
//	}

	Ŧ ClientSocketAwait<T>::await_resume()ε->T{
		ASSERT( _promise && (_promise->Result || _promise->Exception) );
		if( _promise->Exception )
			_promise->Exception->Throw();
		return move( *_promise->Result );
	}
/*
	Ŧ TTask<T>::promise_type::unhandled_exception()ι->void{//TODO consolidate
		try{
			BREAK;
			throw;
		}
		catch( IException& e ){
			e.SetLevel( ELogLevel::Critical );
		}
		catch( nlohmann::json::exception& e ){
			Exception{ SRCE_CUR, move(e), ELogLevel::Critical, "json exception - {}", e.what() };
		}
		catch( std::exception& e ){
			Exception{ SRCE_CUR, move(e), ELogLevel::Critical, "std::exception - {}", e.what() };
		}
		catch( ... ){
			Exception{ SRCE_CUR, ELogLevel::Critical, "unknown exception" };
		}
	}
	*/
}
#undef Φ
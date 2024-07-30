#pragma once
#include <jde/web/client/socket/IClientSocketSession.h>
#include "../usings.h"

namespace Jde::Web::Client{
	α SocketClientReadTag()ι->sp<LogTag>;

	template<class T>
	struct TTimedTask final{
		struct promise_type : IExpectedPromise<TTimedTask<T>,T>{
			ψ Log( fmt::format_string<Args...> m, Args&& ...args )ι->void{
				ResponseMessage = { m.get().data(), m.get().size() };
				MessageArgs.reserve( sizeof...(args) );
				ToVec::Append( MessageArgs, args... );
			}
			sv ResponseMessage;
			vector<string> MessageArgs;
		};
	};

	template<class T>
	struct ClientSocketAwait final : TAwait<T,TTimedTask<T>>{
		using base = TAwait<T,TTimedTask<T>>;
		ClientSocketAwait( string&& request, RequestId requestId, sp<IClientSocketSession> session, SRCE )ι;
		α await_suspend( base::Handle h )ε->void override;
		//α await_suspend( coroutine_handle<typename Http::TTimedTask<T>::promise_type> h )ε->void override;
		α await_resume()ε->T override;
	private:
		string _request;
		const RequestId _requestId;
		sp<IClientSocketSession> _session;
		steady_clock::time_point _start;
	};

	Τ ClientSocketAwait<T>::ClientSocketAwait( string&& request, RequestId requestId, sp<IClientSocketSession> session, SL sl )ι:
		base{ sl },
		_request{ move(request) },
		_requestId{ requestId },
		_session{ session },
		_start{ steady_clock::now() }
	{}

	Ŧ ClientSocketAwait<T>::await_suspend( base::Handle h )ε->void{
	//Ŧ ClientSocketAwait<T>::await_suspend( base::Handle h )ε->void{
		base::await_suspend( h );
		_session->AddTask( _requestId, h );
		_session->Write( move(_request) );
	}

	Ŧ ClientSocketAwait<T>::await_resume()ε->T{
		base::AwaitResume();
		typename base::TPromise* p = base::Promise();
		THROW_IF( !p, "Not Connected" );
		if( auto e = p->MoveError(); e )
			e->Throw();
		ASSERT( p->Value() );
		if( ShouldTrace(SocketClientReadTag()) ){
			const auto msg = sv{Str::ToString(p->ResponseMessage, p->MessageArgs)}.substr( 0, MaxLogLength() );
			Logging::Log( Logging::Message{ELogLevel::Trace, "[{:x}]HttpReceive - {} - {}", base::_sl}, SocketClientReadTag(), _session->Id(), msg, Chrono::ToString(steady_clock::now()-_start) );
		}
		return move( *p->Value() );
	}
}


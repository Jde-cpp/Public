#pragma once
#include <jde/web/client/socket/IClientSocketSession.h>
#include "../usings.h"
//#include "../exports.h"

namespace Jde::Web::Client{

	struct TimedPromiseType{
		ψ Log( const fmt::format_string<Args const&...>&& m2, const Args&... args )ι->void{
			Trace{ ELogTags::SocketClientRead, FWD(m2), FWD(args)... };
		}
		α Log( SessionPK sessionId, steady_clock::time_point start, SL sl)ι->void{
			if( ShouldTrace(ELogTags::SocketClientRead) ){
				const auto msg = sv{Str::ToString(ResponseMessage, MessageArgs)}.substr( 0, MaxLogLength() );
				Trace{ sl, ELogTags::SocketClientRead, "[{:x}]SocketReceive - {} - {}", sessionId, msg, Chrono::ToString( steady_clock::now() - start ) };
			}
		}

		sv ResponseMessage;
		vector<string> MessageArgs;
	};
	struct TimedVoidTask final{
		struct promise_type : VoidPromise<TimedVoidTask>, TimedPromiseType{};
	};

	struct ClientSocketVoidAwait final : VoidAwait<void,TimedVoidTask>{
		using base = VoidAwait<void,TimedVoidTask>;
		ClientSocketVoidAwait( string&& request, RequestId requestId, sp<IClientSocketSession> session, SRCE )ι;
		α Suspend()ι->void override;
		α await_resume()ε->void override;
	private:
		string _request;
		const RequestId _requestId;
		sp<IClientSocketSession> _session;
		steady_clock::time_point _start;
	};

	inline ClientSocketVoidAwait::ClientSocketVoidAwait( string&& request, RequestId requestId, sp<IClientSocketSession> session, SL sl )ι:
		base{ sl }, _request{ move(request) }, _requestId{ requestId }, _session{ session }, _start{ steady_clock::now() }
	{}

	Ξ ClientSocketVoidAwait::Suspend()ι->void{
		_session->AddTask( _requestId, _h );
		_session->Write( move(_request) );
	}

	Ξ ClientSocketVoidAwait::await_resume()ε->void{
		base::AwaitResume();
		typename base::TPromise* p = base::Promise(); THROW_IF( !p, "Not Connected" );
		if( auto e = p->MoveError(); e )
			e->Throw();
		p->Log( _session->Id(), _start, _sl );
	}

	template<class T>
	struct TTimedTask final{
		struct promise_type : IExpectedPromise<TTimedTask<T>,T>, TimedPromiseType{};
	};

	template<class T>
	struct ClientSocketAwait final : TAwait<T,TTimedTask<T>>{
		using base = TAwait<T,TTimedTask<T>>;
		ClientSocketAwait( string&& request, RequestId requestId, sp<IClientSocketSession> session, SRCE )ι;
		α Suspend()ι->void override;
		α await_resume()ε->T override;
	private:
		string _request;
		const RequestId _requestId;
		sp<IClientSocketSession> _session;
		steady_clock::time_point _start;
	};

	Τ ClientSocketAwait<T>::ClientSocketAwait( string&& request, RequestId requestId, sp<IClientSocketSession> session, SL sl )ι:
		base{ sl }, _request{ move(request) }, _requestId{ requestId }, _session{ session }, _start{ steady_clock::now() }
	{}

	Ŧ ClientSocketAwait<T>::Suspend()ι->void{
		_session->AddTask( _requestId, base::_h );
		_session->Write( move(_request) );
	}

	Ŧ ClientSocketAwait<T>::await_resume()ε->T{
		base::AwaitResume();
		typename base::TPromise* p = base::Promise();
		THROW_IF( !p, "Not Connected" );
		if( auto e = p->MoveError(); e )
			e->Throw();
		ASSERT( p->Value() );
		p->Log( _session->Id(), _start, base::_sl );
		return move( *p->Value() );
	}
}


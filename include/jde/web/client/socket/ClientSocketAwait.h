#pragma once
//#include <jde/web/client/socket/IClientSocketSession.h>
#include "../usings.h"
#include <jde/framework/str.h>
#include "../client.h"
#include <jde/framework/chrono.h>

namespace Jde::Web::Client{
	struct IClientSocketSession;
	struct TimedPromiseType{
		ψ Log( const fmt::format_string<Args const&...>&& m2, const Args&... args )ι->void{
			TRACET( ELogTags::SocketClientRead, FWD(m2), FWD(args)... );
		}
		α Log( SessionPK sessionId, steady_clock::time_point start, SL sl)ι->void{
			if( ShouldTrace(ELogTags::SocketClientRead) && ResponseMessage.size() ){
				const auto msg = sv{Str::Format(ResponseMessage, MessageArgs)}.substr( 0, MaxLogLength() );
				LOGSL( ELogLevel::Trace, sl, ELogTags::SocketClientRead, "[{:x}]SocketReceive - {} - {}", sessionId, msg, Chrono::ToString( steady_clock::now() - start ) );
			}
		}

		sv ResponseMessage;
		vector<string> MessageArgs;
	};
	struct TimedVoidTask final{
		struct promise_type : VoidPromise<TimedVoidTask>, TimedPromiseType{};
	};

	struct IClientSocketVoidAwait{
		IClientSocketVoidAwait( string&& request, RequestId requestId, sp<IClientSocketSession> session )ι:
			_request{ move(request) }, _requestId{ requestId }, _session{ session }, _start{ steady_clock::now() }{}

		α Suspend( std::any hCoroutine )ι->void;
	protected:
		α SessionId()ι->SessionPK;
		string _request;
		const RequestId _requestId;
		sp<IClientSocketSession> _session;
		steady_clock::time_point _start;
	};

	struct ClientSocketVoidAwait final : VoidAwait, IClientSocketVoidAwait{
		ClientSocketVoidAwait( string&& request, RequestId requestId, sp<IClientSocketSession> session, SRCE )ι:
			VoidAwait{ sl }, IClientSocketVoidAwait{ move(request), requestId, session }{}
		α Suspend()ι->void{ IClientSocketVoidAwait::Suspend( _h ); }
		α await_resume()ε->void override;
	};

	template<class T>
	struct TTimedTask final{
		struct promise_type : IExpectedPromise<TTimedTask<T>,T>, TimedPromiseType{};
	};

	template<class T>
	struct ClientSocketAwait final : IClientSocketVoidAwait, TAwait<T,TTimedTask<T>>{
		using base = TAwait<T,TTimedTask<T>>;
		ClientSocketAwait( string&& request, RequestId requestId, sp<IClientSocketSession> session, SRCE )ι;
		α Suspend()ι->void{
			IClientSocketVoidAwait::Suspend( base::_h );
		}
		α await_resume()ε->T override;
	};

	Τ ClientSocketAwait<T>::ClientSocketAwait( string&& request, RequestId requestId, sp<IClientSocketSession> session, SL sl )ι:
		IClientSocketVoidAwait{ move(request), requestId, session }, base{ sl }
	{}

	//Ŧ ClientSocketAwait<T>::Suspend()ι->void{
	//	_session->AddTask( _requestId, base::_h );
	//	_session->Write( move(_request) );
	//}

	Ŧ ClientSocketAwait<T>::await_resume()ε->T{
		base::AwaitResume();
		typename base::TPromise* p = base::Promise();
		THROW_IF( !p, "Not Connected" );
		if( auto e = p->MoveExp(); e )
			e->Throw();
		ASSERT( p->Value() );
		p->Log( SessionId(), _start, base::_sl );
		return move( *p->Value() );
	}
}

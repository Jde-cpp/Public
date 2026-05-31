#pragma once
#ifndef CONTEXT_THREAD_H
#define CONTEXT_THREAD_H
#include <jde/fwk/co/Await.h>
#include <functional>
#include <version>

namespace boost::asio{ class io_context; class cancellation_signal; }
#define Φ Γ α
namespace Jde{
	Φ Executor()ι->sp<boost::asio::io_context>;
	Φ Post( function<void()> f )ι->void;
#ifdef __cpp_lib_move_only_function
	Φ PostM( std::move_only_function<void()> f )ι->void;
#endif
	α PostIO( function<void()> f )ι->void;
	Φ Post( VoidAwait::Handle&& h )ι->void;
	Φ Post( VoidAwait::Handle&& h, Exception&& e )ι->void;
	Ŧ Post( T&& value, typename TAwait<T>::Handle h )ι->void;

	namespace Execution{
		Φ AddShutdown( IShutdown* pShutdown )ι->void;
		Φ AddCancelSignal( sp<boost::asio::cancellation_signal> s )ι->void;
		Φ Run()->void;
	}
}
Ŧ Jde::Post( T&& value, typename TAwait<T>::Handle h )ι->void{
	#ifdef __cpp_lib_move_only_function
		PostM( [ v = move(value), h ]() mutable {
			h.promise().Resume( move(v), h );
		} );
	#else
		Post( [ v = move(value), h ]() mutable {
			h.promise().Resume( move(v), h );
		} );
	#endif
}

#endif
#undef Φ
#pragma once
#ifndef CONTEXT_THREAD_H
#define CONTEXT_THREAD_H
#include <jde/coroutine/Await.h>

namespace boost::asio{ class io_context; class cancellation_signal; }
#define Φ Γ α
namespace Jde{
	α Executor()ι->sp<boost::asio::io_context>;
	namespace Execution{
		α AddShutdown( IShutdown* pShutdown )ι->void;
		α AddCancelSignal( sp<boost::asio::cancellation_signal> s )ι->void;
		α Run()->void;
	}
}
#endif
#undef Φ
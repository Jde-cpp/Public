#pragma once
#include "usings.h"
#include <jde/fwk/process/execution.h>

namespace Jde::Web::Client{
	// Owns a single cancellation_signal for one operation timeline. A cancellation_signal
	// supports only one active handler at a time (composed beast ops install/clear the slot
	// repeatedly as they progress), so two operations that can be in flight concurrently —
	// e.g. a read and a write on the same socket — MUST each use a separate ClientCancellation;
	// sharing one signal between them corrupts its handler storage (heap-use-after-free).
	// The signal is registered with the framework executor (Execution::AddCancelSignal) so that,
	// on shutdown, CancellationSignals::Emit() cancels the bound operation and io_context::run()
	// can drain to empty instead of blocking on a pending read/write.
	struct ClientCancellation final{
		ClientCancellation()ι{ Execution::AddCancelSignal( _sig ); }
		// A signal must never outlive the socket whose cancellation handler it holds, otherwise
		// shutdown's Emit() would fire a stale handler against a freed descriptor. Unregister here
		// so the signal's lifetime stays bounded by this object's (i.e. the stream's).
		~ClientCancellation(){ Execution::RemoveCancelSignal( _sig ); }
		ClientCancellation( const ClientCancellation& )=delete;
		ClientCancellation& operator=( const ClientCancellation& )=delete;
		α Slot()ι->net::cancellation_slot{ return _sig->slot(); }
	private:
		sp<net::cancellation_signal> _sig = ms<net::cancellation_signal>();
	};
}

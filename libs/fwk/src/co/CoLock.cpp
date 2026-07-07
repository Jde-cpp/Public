#include <jde/fwk/co/CoLock.h>
#include <boost/container/flat_map.hpp>
#include <jde/fwk/process/execution.h>
#define let const auto
namespace Jde{
	constexpr ELogTags _tags = ELogTags::Locks;

	LockAwait::LockAwait( CoLock& lock )ι:_lock{lock}{}
	LockAwait::~LockAwait(){} //clang error without this.
	α LockAwait::await_ready()ι->bool{
		_guard = _lock.TestAndSet();
		TRACE( "[{:x}]LockAwait ready={}", (uint)&_lock, _guard.has_value() );
		return _guard.has_value();
	}

	α LockAwait::await_resume()ι->CoGuard{
		return _guard ? CoGuard{move(*_guard)} : base::await_resume();
	}

	α LockAwait::Suspend()ι->void{
		if( (_guard = _lock.Push(_h)) ){
			TRACE( "[{:x}]LockAwait::Suspend resume", (uint)&_lock );
			_h.resume();
		}
	}


	CoGuard::CoGuard( CoLock& rhs )ι:_lock{&rhs}{
	}

	CoGuard::CoGuard( CoGuard&& rhs )ι:_lock{ rhs._lock }{
		rhs._lock = nullptr;
	}
	α CoGuard::operator=( CoGuard&& rhs )ι->CoGuard&{
		if( this!=&rhs ){
			if( _lock )
				_lock->Clear();//release the held lock - overwriting would leave it locked forever.
			_lock = rhs._lock;
			rhs._lock = nullptr;
		}
		return *this;
	}
	CoGuard::~CoGuard(){
		if( _lock ){
			TRACE( "[{:x}]~CoGuard", (uint)_lock );
			_lock->Clear();
		}
	}
	α CoGuard::unlock()ι->void{ if( _lock ){ _lock->Clear(); _lock=nullptr; } }//idempotent - safe on a moved-from or already-unlocked guard.

	α CoLock::TestAndSet()ι->optional<CoGuard>{
		lg l{ _mutex };
		return _locked.test_and_set( std::memory_order_acquire ) ? optional<CoGuard>{} : CoGuard{ *this };
	}
	α CoLock::Push( LockAwait::Handle h )ι->optional<CoGuard>{
		lg l{ _mutex };
		auto guard = _locked.test_and_set(std::memory_order_acquire) ? optional<CoGuard>{} : CoGuard{ *this };
		if( !guard ){
			_queue.push( move(h) );
			TRACE( "[{:x}]CoLock::Push Locked queue size={}", (uint)this, _queue.size() );
		}
		else
			TRACE( "[{:x}]CoLock::Push unlocked", (uint)this, _queue.size() );
		return guard;
	}
	α CoLock::Clear()ι->void{
		LockAwait::Handle h{};
		{
			lg _{ _mutex };
			if( _queue.size() ){
				TRACE( "[{:x}]CoLock::Clear resuming queueSize={}", (uint)this, _queue.size() );
				h = _queue.front();
				_queue.pop();
				h.promise().SetValue( CoGuard{*this} );//ownership transfers - _locked stays set.
			}
			else{
				_locked.clear();
				TRACE( "[{:x}]CoLock::Clear locked={}", (uint)this, _locked.test() );
			}
		}
		if( h )//resume off-mutex & deferred: the waiter's release re-enters Clear, which would deadlock on _mutex if resumed inline here.
			Post( [h](){ h.resume(); } );
	}
}
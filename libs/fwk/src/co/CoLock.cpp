#include <jde/fwk/co/CoLock.h>
#include <boost/container/flat_map.hpp>
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
		_lock = move( rhs._lock );
		rhs._lock = nullptr;
		return *this;
	}
	CoGuard::~CoGuard(){
		if( _lock ){
			TRACE( "[{:x}]~CoGuard", (uint)_lock );
			_lock->Clear();
		}
	}
	α CoGuard::unlock()ι->void{ _lock->Clear(); _lock=nullptr; }

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
		lg _{ _mutex };
		if( _queue.size() ){
			TRACE( "[{:x}]CoLock::Clear resuming queueSize={}", (uint)this, _queue.size() );
			auto h = _queue.front();
			_queue.pop();
			h.promise().SetValue( CoGuard{*this} );
			h.resume();
		}
		else{
			_locked.clear();
			TRACE( "[{:x}]CoLock::Clear locked={}", (uint)this, _locked.test() );
		}
	}
}
namespace Jde::Threading{
/*	static boost::container::flat_map<string,sp<shared_mutex>> _mutexes;
	mutex _mutex;
	unique_lock<shared_mutex> UniqueLock( str key )ι
	{
		unique_lock l{_mutex};

		auto p = _mutexes.find( key );
		auto pKeyMutex = p == _mutexes.end() ? _mutexes.emplace( key, ms<shared_mutex>() ).first->second : p->second;
		for( auto pExisting = _mutexes.begin(); pExisting != _mutexes.end();  )
			pExisting = pExisting->first!=key && pExisting->second.use_count()==1 && pExisting->second->try_lock() ? _mutexes.erase( pExisting ) : std::next( pExisting );
		l.unlock();
		TRACE( "UniqueLock( '{}' )", key );
		return unique_lock{ *pKeyMutex };
	}
*/
}
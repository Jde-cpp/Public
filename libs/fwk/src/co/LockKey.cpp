#include <jde/fwk/co/LockKey.h>
#include <deque>
#include <jde/fwk/process/execution.h>

#define let const auto
namespace Jde{

	constexpr ELogTags _tags = ELogTags::Locks;
	flat_map<string,std::deque<std::variant<uint,coroutine_handle<>>>> _coLocks; mutex _coLocksMutex;
	atomic<uint> _lockIndex;

	α LockKeyAwait::await_ready()ι->bool{
		lg _( _coLocksMutex );
		auto& locks = _coLocks.try_emplace( Key ).first->second;
		_index = ++_lockIndex;
		locks.push_back( _index );
		let ready = locks.size()==1;
		TRACE( "({})LockKeyAwait::await_ready={} size={} 0={}", Key, ready, locks.size(), locks[0].index() ? "await" : "ready" );
		return locks.size()==1;
	}
	α LockKeyAwait::Suspend()ι->void{
		_coLocksMutex.lock(); ASSERT( _coLocks.find(Key)!=_coLocks.end() );
		auto& locks = _coLocks.find( Key )->second;
		if( locks.size()==1 ){//if other locks freed after await_ready
			_coLocksMutex.unlock();
			_h.resume();
			return;
		}
		ASSERT( locks.size() );
		for( int i=(int)locks.size()-1; !_waitHandle && i>=0; --i ){
			if( locks[i].index()==0 && get<uint>(locks[i])==_index )
				locks[i] = _waitHandle = _h;
		}
		_coLocksMutex.unlock();
		ASSERT( _waitHandle );
		TRACE( "({})LockKeyAwait::await_suspend size={}", Key, locks.size() );
	}
	α LockKeyAwait::await_resume()ι->CoLockGuard{
		return _waitHandle ? CoLockGuard{ Key, _waitHandle } : CoLockGuard{ Key, _index };
	}

	CoLockGuard::CoLockGuard( string key, std::variant<uint,coroutine_handle<>> h )ι:
		Handle{h},
		Key{ move(key) }{
			TRACE( "({})CoLockGuard() index={}", Key, h.index() );
	}
	CoLockGuard::CoLockGuard( CoLockGuard&& rhs )ι:Handle{move(rhs.Handle)}, Key{move(rhs.Key)}{
		rhs.Handle = nullptr;
	}

	α CoLockGuard::operator=( CoLockGuard&& x )ι->CoLockGuard&{
		Handle = x.Handle;
		x.Handle = {};
		Key = move( x.Key );
		x.Key.clear();
		return *this;
	}

	CoLockGuard::~CoLockGuard(){
		if( Key.empty() )
			return;

		lg _( _coLocksMutex );
		ASSERT( _coLocks.find(Key)!=_coLocks.end() && _coLocks.find(Key)->second.size() );
		auto& locks = _coLocks.find( Key )->second; //ASSERT( locks[0]==Handle );
		if( locks[0]!=Handle )
			TRACE( "({})~CoLockGuard - unexpected handle at front of queue. locks={} vs handle={} ", Key, locks[0].index() ? "await" : "ready", Handle.index()  ? "await" : "ready" );
		TRACE( "({})~CoLockGuard - pop front.", Key, locks.size() );
		locks.pop_front();
		if( locks.size() ){
			if( locks.front().index()==1 )
				Post( move(get<1>(locks.front())) );
			else
				TRACE( "({})~CoLockGuard - size={}, next is await_ready, should continue.", Key, locks.size() );
		}
		TRACE( "~CoLockGuard( {} )", Key );
	}
}
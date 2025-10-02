#include <jde/fwk/co/LockKey.h>
#include <deque>
#include <jde/fwk/process/execution.h>

#define let const auto
namespace Jde{

	constexpr ELogTags _tags = ELogTags::Locks;
	flat_map<string,std::deque<std::variant<LockKeyAwait*,coroutine_handle<>>>> _coLocks; mutex _coLocksLock;

	α LockKeyAwait::await_ready()ι->bool{
		lg _( _coLocksLock );
		auto& locks = _coLocks.try_emplace( Key ).first->second;
		locks.push_back( this );
		let ready = locks.size()==1;
		TRACE( "({})LockKeyAwait::await_ready={} size={}", Key, ready, locks.size() );
		return locks.size()==1;
	}
	α LockKeyAwait::Suspend()ι->void{
		_coLocksLock.lock(); ASSERT( _coLocks.find(Key)!=_coLocks.end() );
		auto& locks = _coLocks.find( Key )->second;
		if( locks.size()==1 ){//if other locks freed after await_ready
			_coLocksLock.unlock();
			_h.resume();
		}
		else{
			for( uint i=0; !Handle && i<locks.size(); ++i ){
				if( locks[i].index()==0 && get<LockKeyAwait*>(locks[i])==this )
					locks[i] = Handle = _h;
			}
			_coLocksLock.unlock();
			ASSERT( Handle );
			TRACE( "({})LockKeyAwait::await_suspend size={}", Key, locks.size() );
		}
	}
	α LockKeyAwait::await_resume()ι->CoLockGuard{
		return Handle ? CoLockGuard{ Key, Handle } : CoLockGuard{ Key, this };
	}

	CoLockGuard::CoLockGuard( string key, std::variant<LockKeyAwait*,coroutine_handle<>> h )ι:
		Handle{h},
		Key{ move(key) }{
			TRACE( "({})CoLockGuard() index={}", Key, h.index() );
	}
	CoLockGuard::CoLockGuard( CoLockGuard&& rhs )ι:Handle{move(rhs.Handle)}, Key{move(rhs.Key)}{
		rhs.Handle = nullptr;
	}

	α CoLockGuard::operator=( CoLockGuard&& rhs )ι->CoLockGuard&{
		Handle = move( rhs.Handle );
		rhs.Handle = (LockKeyAwait*)nullptr;
		Key = move( rhs.Key );
		rhs.Key.clear();
		return *this;
	}

	CoLockGuard::~CoLockGuard(){
		if( Key.empty() )
			return;

		lg _( _coLocksLock );
		ASSERT( _coLocks.find(Key)!=_coLocks.end() && _coLocks.find(Key)->second.size() );
		auto& locks = _coLocks.find( Key )->second; ASSERT( locks[0]==Handle );
		locks.pop_front();
		if( locks.size() ){
			if( locks.front().index()==1 )
				Post( move(get<1>(locks.front())) );
				//CoroutinePool::Resume( move(get<1>(locks.front())) );
			else
				TRACE( "({})CoLockGuard - size={}, next is awaitable, should continue.", Key, locks.size() );
		}
		TRACE( "~CoLockGuard( {} )", Key );
	}
}
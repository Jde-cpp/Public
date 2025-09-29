#pragma once
#ifndef MUTEX_H
#define MUTEX_H
#include <shared_mutex>
#include <queue>
#include <boost/core/noncopyable.hpp>
#include <jde/framework/co/Await.h>
#include <jde/framework/process/thread.h>

#define Φ Γ auto
namespace Jde{
	struct CoLock;
	struct Γ CoGuard{
		CoGuard()ι{ ASSERT(false); }
		CoGuard( CoGuard&& lock )ι;
		~CoGuard();
		α operator=( CoGuard&& )ι->CoGuard&;
		α unlock()ι->void;
	private:
		CoGuard( CoLock& lock )ι;
		CoGuard( const CoGuard& )ι = delete;
		α operator=( const CoGuard& )ι->CoGuard& = delete;
		CoLock* _lock{};
		friend CoLock; friend class LockAwait;
	};

	class Γ LockAwait : public TAwait<CoGuard>{
		using base=TAwait<CoGuard>;
	public:
		LockAwait( CoLock& lock )ι;
		~LockAwait();
		α await_ready()ι->bool override;
		α Suspend()ι->void override;
		α await_resume()ι->CoGuard override;
	private:
		CoLock& _lock;
		optional<CoGuard> _guard;
	};

	struct Γ CoLock{
		α Lock(){ return LockAwait{*this}; }
	private:
		α TestAndSet()ι->optional<CoGuard>;
		α Push( LockAwait::Handle h )ι->optional<CoGuard>;
		α Clear()ι->void;

		std::queue<LockAwait::Handle> _queue; mutex _mutex;
		atomic_flag _locked;
		friend class LockAwait; friend struct CoGuard;
	};
}
#undef Φ
#endif
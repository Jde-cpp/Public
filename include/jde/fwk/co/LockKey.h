#pragma once
#include "boost/noncopyable.hpp"
#include <jde/fwk/co/Await.h>

namespace Jde{
	//using namespace Coroutine;
	struct LockKeyAwait;
	struct Γ CoLockGuard final{
		CoLockGuard( string Key, std::variant<uint,coroutine_handle<>> )ι;
		CoLockGuard( CoLockGuard&& rhs )ι;
		α operator=( CoLockGuard&& rhs )ι->CoLockGuard&;
		~CoLockGuard();
	private:
		variant<uint,coroutine_handle<>> Handle;
		string Key;
	};

	struct Γ LockKeyAwait final : TAwait<CoLockGuard>, boost::noncopyable{
		using base=TAwait<CoLockGuard>;
		LockKeyAwait( string key )ι:Key{move(key)}{}
		~LockKeyAwait(){ TRACET( ELogTags::Locks, "({})~LockKeyAwait", Key ); }

		α await_ready()ι->bool override;
		α Suspend()ι->void override;
		α await_resume()ι->CoLockGuard override;
	private:
		uint _index;
		base::Handle _waitHandle{nullptr};
		const string Key;
	};
}
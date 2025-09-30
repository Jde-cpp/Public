#pragma once
#include "boost/noncopyable.hpp"
#include <jde/fwk/co/Await.h>

namespace Jde{
	//using namespace Coroutine;
	struct LockKeyAwait;
	struct Γ CoLockGuard final{
		CoLockGuard( string Key, std::variant<LockKeyAwait*,coroutine_handle<>> )ι;
		CoLockGuard( CoLockGuard&& rhs )ι;
		α operator=( CoLockGuard&& rhs )ι->CoLockGuard&;
		~CoLockGuard();
	private:
		variant<LockKeyAwait*,coroutine_handle<>> Handle;
		string Key;
	};

	struct Γ LockKeyAwait final : TAwait<CoLockGuard>, boost::noncopyable{
		using base=TAwait<CoLockGuard>;
		LockKeyAwait( string key )ι:Key{move(key)}{}

		α await_ready()ι->bool override;
		α Suspend()ι->void override;
		α await_resume()ι->CoLockGuard override;
	private:
		base::Handle Handle{nullptr};
		const string Key;
	};
}
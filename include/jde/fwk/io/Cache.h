#pragma once
#include <any>
#include "jde/fwk/usings.h"
#include <jde/fwk/process/process.h>
#include <jde/fwk/str.h>
#include <jde/fwk/settings.h>
#include <jde/fwk/utils/collections.h>
#include <jde/fwk/log/log.h>

#define Φ Γ α

namespace Jde::Cache{
	Φ Init()ι->void;
	Φ DefaultDuration()ι->steady_clock::duration;

	Ŧ Get( str id )ι->sp<const T>;
	Ŧ Set( str name, T value, optional<steady_clock::duration> duration = DefaultDuration() )ι->sp<const T>;
	Ŧ Set( str name, sp<const T> value, optional<steady_clock::duration> duration = DefaultDuration() )ι->sp<const T>;//zero-copy - the cache shares the caller's instance.
	Φ Shutdown( bool terminate, SRCE )ι->void;
	Φ Clear( str id )ι->bool;
	namespace Internal{
		Φ Get( str id )ι->sp<const std::any>;
		Φ Set( str id, sp<std::any> p, optional<steady_clock::duration> duration = DefaultDuration() )ι->sp<const std::any>;
	}
}
namespace Jde{
	Ŧ Cache::Get( str id )ι->sp<const T>{
		auto p = Cache::Internal::Get( id );
		if( auto* tptr = p ? std::any_cast<std::shared_ptr<const T>>(p.get()) : nullptr )
			return *tptr;
		else{
			DBGT( ELogTags::Cache, "Cache miss for {}", id );
			return nullptr;
		}
	}
	Ŧ Cache::Set( str id, sp<const T> value, optional<steady_clock::duration> duration )ι->sp<const T>{
		DBGT( ELogTags::Cache, "Cache set for {}", id );
		Internal::Set( id, ms<std::any>(value), duration );
		return value;
	}
	Ŧ Cache::Set( str id, T value, optional<steady_clock::duration> duration )ι->sp<const T>{
		return Set<T>( id, sp<const T>{ms<const T>(std::move(value))}, duration );
	}
}
#undef Φ
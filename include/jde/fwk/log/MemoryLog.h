#pragma once
#include <jde/fwk/utils/Vector.h>
#include "ILogger.h"
#include "jde/fwk/usings.h"
#define Φ Γ auto

namespace Jde::Logging{
	Φ ClearMemory()ι->void;
	Φ Find( StringMd5 entryId )ι->vector<Logging::Entry>;
	Φ Find( function<bool(const Logging::Entry&)> f )ι->vector<Logging::Entry>;

	struct MemoryLog final : ILogger{
		MemoryLog()ι:ILogger{ jobject{{"tags",jobject{{"default", "Trace"}}}} }{}
		α Shutdown( bool /*terminate*/, SL )ι->void override{ _entries.clear(); }
		α Write( const Entry& m )ι->void override;
		α Write( const Entry& m, uint32 /*appPK*/, uint32 /*instancePK*/ )ι->void override{ Write(m); }
		α Write( ILogger& logger )ι->void;
		α Name()Ι->sv override{ return "MemoryLog"; }
		α SetMinLevel( ELogLevel /*level*/ )ι->void override{}
		α Clear()ι->void{ _entries.clear(); }
		α Find( StringMd5 id )ι->vector<Entry>;
		Φ Find( string text )ι->vector<Entry>;
		α Find( function<bool(const Entry&)> f )ι->vector<Entry>;
	private:
		Vector<Entry> _entries;
	};
}
#undef Φ
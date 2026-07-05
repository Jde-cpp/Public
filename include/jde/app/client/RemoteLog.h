#pragma once
#include <jde/fwk/co/Timer.h>
#include <jde/fwk/log/ILogger.h>
#include "../log/ProtoLog.h"

namespace Jde::App::Client{
	struct IAppClient;
	struct RemoteLog final : Logging::ILogger, noncopyable{
		RemoteLog( const jobject& settings, sp<IAppClient> client )ι;
		~RemoteLog();
		Ω Init( sp<IAppClient> client )ι->void;
		α Shutdown( bool terminate=false, SRCE )ι->void override;
		α Write( const Logging::Entry& m )ι->void override;
		α Write( const Logging::Entry& m, uint32 /*appPK*/, uint32 /*instancePK*/ )ι->void override{ Write(m); }
		α Name()Ι->sv override{ return "RemoteLog"; }
	private:
		α ResetTimer()ι->void;
		α Send()ι->void;
		α Start( sp<IAppClient> client )ι->void;
		α StartTimer()ι->TimerAwait::Task;
		ProtoLogCache _cache;
		sp<IAppClient> _client;
		Duration _delay;
		mutex _mutex;
		static constexpr ELogTags _tags{ ELogTags::ExternalLogger | ELogTags::App };
		up<DurationTimer> _timer;
		vector<Logging::Entry> _entries;
	};
}
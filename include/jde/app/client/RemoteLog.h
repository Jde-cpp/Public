#pragma once
#include <jde/fwk/co/Timer.h>
#include <jde/fwk/log/ILogger.h>
#include "../log/ProtoLog.h"

namespace Jde::App::Client{
	struct RemoteLog final : Logging::ILogger, boost::noncopyable{
		RemoteLog()ι:RemoteLog( jobject{Settings::FindDefaultObject("/logging/proto")} ){}
		RemoteLog( const jobject& settings )ι;
		α Shutdown( bool terminate )ι->void override;
		α Write( const Logging::Entry& m )ι->void override;
		α Name()Ι->string override{ return "RemoteLog"; }
	private:
		α ResetTimer()ι->void;
		α Send()->void;
		α StartTimer()ι->VoidAwait::Task;
		ProtoLogCache _cache;
		Duration _delay;
		mutex _mutex;
		static constexpr ELogTags _tags{ ELogTags::ExternalLogger | ELogTags::App };
		up<DurationTimer> _timer;
		vector<Logging::Entry> _entries;
	};
}
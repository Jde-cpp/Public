#pragma once
#include <chrono>
#include <jde/framework/log/ILogger.h>
#include <jde/framework/co/Timer.h>
#include <jde/framework/settings.h>

namespace Jde::App{
	struct ProtoLog final : Logging::ILogger, boost::noncopyable{
		ProtoLog()ι:ProtoLog( jobject{Settings::FindDefaultObject("/logging/proto")} ){}
		ProtoLog( const jobject& settings )ι;
		α Shutdown( bool terminate )ι->void override;
		α Write( const Logging::Entry& m )ι->void override;
		α Name()Ι->string override{ return "ProtoLog"; }
		α SetMinLevel( ELogLevel /*level*/ )ι->void override{}
		α Load()ι->vector<App::Log::Proto::FileEntry>;
	private:
		α AddString( uuid id, sv str )ι->void;
		α AddString( uuid id, sv str, std::deque<uuid>& cache )ι->void;
		α AddArguments( const vector<string>& args, ::google::protobuf::RepeatedPtrField<std::string> ids )ι->void;
		α Save()ι->VoidAwait::Task;
		α StartTimer()->VoidAwait::Task;
		α ResetTimer()->void;

		std::deque<uuid> _args;
		Duration _delay;
		const uint16 _delaySize{8096};
		//Vector<Logging::Entry> _entries;
		mutex _mutex;
		fs::path _path;
		std::deque<uuid> _strings;
		ELogTags _tags{ ELogTags::ExternalLogger | ELogTags::IO };
		up<DurationTimer> _timer;
		const std::chrono::time_zone& _tz;
		vector<byte> _toSave;
	};
}

#pragma once
#ifndef SPDLOG
#define SPDLOG
#include <jde/fwk/log/ILogger.h>
#include <spdlog/common.h>
#include <spdlog/logger.h>

#define FormatString const fmt::format_string<Args const&...>
#define ARGS const Args&

namespace Jde::Logging{
	Ξ ToSpdSL( SL sl )ι->spdlog::source_loc{ return {sl.file_name(), (int)sl.line(), sl.function_name()}; }
	struct SpdLog final : ILogger{
		SpdLog()ι;
		ψ Write( ELogLevel level, SL sl, FormatString&& m, ARGS... args )ε{
			_logger.log( ToSpdSL(sl), (spdlog::level::level_enum)level, FWD(m), FWD(args)... );
		}
		α Shutdown( bool /*terminate*/ )ι->void override{ _logger.flush(); }
		α Name()Ι->string override{ return _logger.name(); }
		α SetMinLevel( ELogLevel /*level*/ )ι->void override{}
		α Write( const Entry& m )ι->void override{
			_logger.log( m.SourceLocation(), (spdlog::level::level_enum)m.Level, m.Message() );
		}
	private:
		spdlog::logger _logger;
	};
}
#undef FormatString
#undef ARGS
#endif
#pragma once
#ifndef SPDLOG
#define SPDLOG
#include "ILogger.h"

#define FormatString const fmt::format_string<Args const&...>
#define ARGS const Args&

namespace Jde::Logging{
	struct SpdLog final : ILogger{
		SpdLog()ι;
		ψ Write( ELogLevel level, const spdlog::source_loc& sl, FormatString&& m, ARGS... args )ε{
			_logger.log( sl, (spdlog::level::level_enum)level, FWD(m), FWD(args)... );
		}
		α Name()ι->string override{ return _logger.name(); }
		α SetMinLevel( ELogLevel /*level*/ )ι->void override{}
		α Write( const Entry& m )ι->void override{
			_logger.log( m.SourceLocation(), (spdlog::level::level_enum)m.Level, m.Message() );
		}
	private:
		spdlog::logger _logger;
	};
	Ξ ToSpdSL( SL sl )ι->spdlog::source_loc{ return {sl.file_name(), (int)sl.line(), sl.function_name()}; }
}
#undef FormatString
#undef ARGS
#endif
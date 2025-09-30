#include <jde/fwk/log/logTags.h>
#include <jde/fwk/log/ILogger.h>
#include <jde/fwk/settings.h>
#include <jde/fwk/str.h>
#define let const auto

namespace Jde{
	constexpr std::array<sv,29> ELogTagStrings = {"none",
		"access", "app", "cache", "client", "crypto", "dbDriver", "exception", "externalLogger",
		"http", "io", "locks", "parsing", "pedantic", "ql", "read", "scheduler",
		"server", "sessions", "settings", "shutdown", "socket", "sql", "startup", "subscription",
		"test", "threads", "write"
	};

	namespace Logging{
		Ω min( ELogLevel a, ELogLevel b )ι->ELogLevel{
			using enum ELogLevel;
			return a==NoLog || b==NoLog ? std::max( a, b ) : std::min( a, b );
		}
	}
	LogTags	_cumulative{ ELogLevel::Trace };
	vector<up<Logging::ITagParser>> _tagParsers;
	α Logging::AddTagParser( up<ITagParser>&& tagParser )ι->void{ _tagParsers.emplace_back( std::move(tagParser) ); }

	α Logging::UpdateCumulative( const vector<up<Logging::ILogger>>& loggers )ι->void{
		optional<LogTags> cumulative;
		for( let& logger : loggers ){
			if( !cumulative ){
				cumulative = *logger;
				continue;
			}
			cumulative->_minLevel = min( cumulative->MinLevel(), logger->MinLevel() );
			logger->ConfiguredTags.cvisit_all( [&cumulative](let& kv){
				cumulative->ConfiguredTags.insert_or_visit( kv, [&kv]( auto& cumulativeValues ){
					cumulativeValues.second = min( cumulativeValues.second, kv.second );
				} );
			} );
		}
		_cumulative = cumulative.value_or( LogTags{ELogLevel::NoLog} );
	}

	α Logging::ShouldLog( ELogLevel level, ELogTags tags )ι->bool{
		return _cumulative.ShouldLog( level, tags );
	}
}

#pragma warning(disable:4334)
α Jde::ToString( ELogTags tags )ι->string{
	if( tags==ELogTags::None )
		return string{ELogTagStrings[0]};
	vector<string> tagStrings;
	for( uint i=1; i<ELogTagStrings.size(); ++i ){
		if( (uint)tags & (uint)(1ul<<(i-1ul)) )
			tagStrings.push_back( string{ELogTagStrings[i]} );
	}
	for( let& parser : _tagParsers ){
		auto additional = parser->ToString(tags);
		if( additional.size() )
			tagStrings.push_back( additional );
	}
	return tagStrings.empty() ? string{ELogTagStrings[0]} : Str::Join( tagStrings, "." );
}

α Jde::ToLogTags( sv name )ι->ELogTags{
	auto flags = Str::Split( name, "." );
	ELogTags y{};
	for( let& subName : flags ){
		ELogTags tag{};
		if( auto i = (ELogTags)std::distance(ELogTagStrings.begin(), find(ELogTagStrings, subName)); i<(ELogTags)ELogTagStrings.size() )
			tag |= (ELogTags)( 1ul<<(underlying(i)-1) );
		else{
			for( uint i=0; i<_tagParsers.size() && empty(tag); ++i )
				tag |= _tagParsers[i]->ToTag( string{subName} );
		}
		if( empty(tag) )
			WARNT( ELogTags::Settings, "Unknown tag '{}'", subName );
		y |= tag;
	}
	return y;
}

namespace Jde{
	Ω parseTags( const jobject& o )ι->concurrent_flat_map<ELogTags,ELogLevel>{
		concurrent_flat_map<ELogTags,ELogLevel> y;
		let jtags = Json::FindObject( o, "tags" );
		for( uint i=0; jtags && i<LogLevelStrings().size(); ++i ){
			let level = LogLevelStrings()[i];
			auto tags = Json::FindArray( *jtags, Str::ToLower(level) );
			if( !tags )
				continue;
			for( let& jtag : *tags ){
				if( let tagName = jtag.try_as_string(); tagName ){
					let tag = ToLogTags( *tagName );
					if( !empty(tag) )
						y.emplace( tag, (ELogLevel)i );
				}
			}
		}
		return y;
	}

	LogTags::LogTags( jobject o )ι:
		ConfiguredTags{ parseTags(o) },
		ExtrapolatedTags{ ConfiguredTags },
		_defaultLevel{ Json::FindEnum<ELogLevel>( o, "default", ToLogLevel ).value_or(ELogLevel::Information) }
	{}

	Ω split( ELogTags tags )ι->vector<ELogTags>{
		vector<ELogTags> result;
		for( uint i=0; i<ELogTagStrings.size(); ++i ){
			let flag = (ELogTags)( 1ul<<(i-1) );
			if( !empty(tags & flag) )
				result.push_back( flag );
		}
		return result;
	}
	α LogTags::MinLevel( ELogTags tags )Ι->ELogLevel{
		optional<ELogLevel> level;
		if( ExtrapolatedTags.cvisit(tags, [&](let& kv){level = kv.second;}) )
			return *level;
		let pedantic = !empty( tags & ELogTags::Pedantic );
		if( !pedantic ){
			vector<ELogTags> individual = split( tags );
			if( individual.size()>1 ){
				uint matches{};
				ExtrapolatedTags.cvisit_while( [&level,&matches,tags,count=individual.size()](let& kv ){
					if( empty(kv.first & tags) )
						return true;
					if( auto iterCount = split( tags ).size(); iterCount>matches ){
						level = kv.second;
						matches = iterCount;
					}
					return matches+1<count;
				} );
			}
			TRACET( ELogTags::App | ELogTags::Pedantic, "[{}]tag: {}, minLevel: {}", Name(), Jde::ToString(tags), level ? Jde::ToString(*level) : "{default}" );
		}
		if( !level )
			level = pedantic ? ELogLevel::NoLog : _defaultLevel;
		ExtrapolatedTags.emplace( tags, *level );
		return *level;
	}

	α LogTags::SetLevel( ELogTags tags, ELogLevel level )ι->void{
		ConfiguredTags.insert_or_assign( tags, level );
		ExtrapolatedTags = ConfiguredTags;
		UpdateCumulative( Logging::Loggers() );
	}

	α LogTags::ShouldLog( ELogLevel level, ELogTags tags )Ι->bool{
		let configuredMin = level==ELogLevel::NoLog ? ELogLevel::NoLog : MinLevel( tags );
		return configuredMin!=ELogLevel::NoLog && configuredMin <= level;
	}

	α LogTags::ToString()ι->string{
		flat_map<ELogLevel, vector<string>> levels;
		ConfiguredTags.cvisit_all( [&]( let& kv ){
			levels.try_emplace( kv.second, vector<string>{} ).first->second.push_back( Jde::ToString(kv.first) );
		});
		string y; y.reserve(1024);
		for( auto& [level, tags] : levels )
			y += Ƒ( "[{}]: {}\n", FromEnum(LogLevelStrings(), level), Str::Join(tags, ",") );
		if( y.size() )
			y.pop_back();
		return y;
	}
}

α Jde::ShouldTrace( ELogTags tags )ι->bool {
	return _cumulative.MinLevel( tags ) == ELogLevel::Trace;
}
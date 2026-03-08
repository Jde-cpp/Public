#include <jde/fwk/log/logTags.h>
#include <jde/fwk/log/ILogger.h>
#include <jde/fwk/settings.h>
#include <jde/fwk/str.h>
#define let const auto

namespace Jde{
	constexpr std::array<sv,29> ELogTagStrings = { "none",
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
	up<LogTags>	_cumulative = mu<LogTags>( ELogLevel::Trace );
	vector<up<Logging::ITagParser>> _tagParsers;
	α Logging::AddTagParser( up<ITagParser>&& tagParser )ι->void{ _tagParsers.emplace_back(std::move(tagParser)); }

	α Logging::UpdateCumulative( const vector<up<Logging::ILogger>>& loggers )ι->void{
		up<LogTags> cumulative;
		for( let& logger : loggers ){
			if( !cumulative ){
				cumulative = mu<LogTags>( *logger );
				continue;
			}
			*cumulative+=*logger;
		}
		if( cumulative )
			_cumulative = move( cumulative );
		if( auto l = _cumulative->MinLevel(ELogTags::Settings); l>=ELogLevel::Trace )
			Logging::Log( ELogLevel::Trace, (ELogTags)ELogTags::Settings, SRCE_CUR, "Cumulative: {}", _cumulative->ToString() );
	}

	α Logging::ShouldLog( ELogLevel level, ELogTags tags )ι->bool{
		return _cumulative->ShouldLog( level, tags );
	}
	α Logging::Tags()ι->flat_map<string,uint>{
		flat_map<string,uint> y;
		for( uint i=1; i<ELogTagStrings.size(); ++i )
			y.emplace( string{ELogTagStrings[i]}, 1ul<<(i-1) );
		for( let& parser : _tagParsers ){
			let additional = parser->Tags();
			y.insert( additional.begin(), additional.end() );
		}
		return y;
	}
}

#pragma warning( disable:4334 )
α Jde::ToArray( ELogTags tags )ι->jarray{
	if( tags==ELogTags::None )
		return { string{ELogTagStrings[0]} };
	jarray tagStrings;
	for( uint i=1; i<ELogTagStrings.size(); ++i ){
		if( (uint)tags & (uint)(1ul<<(i-1ul)) )
			tagStrings.push_back( jstring{ELogTagStrings[i]} );
	}
	for( let& parser : _tagParsers ){
		auto additional = parser->ToString( tags );
		if( additional.size() )
			tagStrings.push_back( jstring{additional} );
	}
	return tagStrings.empty() ? jarray{ jstring{ELogTagStrings[0]} } : tagStrings;
}
α Jde::ToString( ELogTags tags, bool outputArray )ι->string{
	auto array = ToArray(tags);
	if( outputArray )
		return serialize( move(array) );
	else{
		string result; result.reserve(array.size()*12);
		for( auto&& item : array ){
			if( !result.empty() )
				result += "_";
			result += move(item.as_string());
		}
		return result;
	}
}

α Jde::ToLogTags( sv name )ι->ELogTags{
	auto flags = Str::Split( name, "_" );
	if( name=="default" )
		return ELogTags::None;
	ELogTags y{};
	for( let& subName : flags ){
		ELogTags tag{};
		if( auto i = (ELogTags)std::distance(ELogTagStrings.begin(), find(ELogTagStrings, subName)); i<(ELogTags)ELogTagStrings.size() )
			tag |= ( ELogTags )( 1ul<<(underlying(i)-1) );
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
α Jde::ToLogTags( jvalue v )ι->ELogTags{
	if( let s = v.try_as_string(); s )
		return ToLogTags( (sv)*s );
	else if( let arr = v.try_as_array(); arr ){
		ELogTags y{};
		for( let& item : *arr ){
			if( let s = item.try_as_string(); s ){
				y |= ToLogTags( (sv)*s );
			}
			else
				WARNT( ELogTags::Settings, "Expected string in tags array but got {}", Json::Kind(item.kind()) );
		}
		return y;
	}
	else{
		WARNT( ELogTags::Settings, "Expected string or array for tags but got {}", Json::Kind(v.kind()) );
		return ELogTags::None;
	}
}

namespace Jde{
	Ṫ parseTags( const jobject& o )ι->T{
		T y;
		for( auto&& [tagName, level] : o ){
			if( tagName=="default" )
				y.insert_or_assign( ELogTags::None, ToLogLevel(level.get_string()) );
			else{
				let tag = ToLogTags( (sv)tagName );
				if( !empty(tag) && level.is_string() )
					y.insert_or_assign( tag, ToLogLevel(level.get_string()) );
			}
		}
		return y;
	}

	LogTags::LogTags( jobject o )ι:
		_configuredTags{ parseTags<concurrent_flat_map<ELogTags,ELogLevel>>(Json::FindDefaultObject(o, "tags")) },
		ExtrapolatedTags{ _configuredTags }{
		_configuredTags.erase_if( ELogTags::None, [&](let& kv ){ _defaultLevel= kv.second; return true; } );
	}
	LogTags::LogTags( const LogTags& x )ι:
		_configuredTags{ x._configuredTags },
		ExtrapolatedTags{ _configuredTags },
		_minLevel{ x._minLevel },
		_defaultLevel{ x._defaultLevel }
	{}
	α LogTags::operator+=( const LogTags& x )ι->LogTags&{
		_minLevel = Logging::min( _minLevel, x._minLevel );
		x._configuredTags.cvisit_all( [this](let& kv){
			this->_configuredTags.insert_or_visit( kv, [&kv](auto& cumulativeValues){
				cumulativeValues.second = Logging::min( cumulativeValues.second, kv.second );
			} );
		} );
		ExtrapolatedTags = _configuredTags;
		return *this;
	}
	α LogTags::SetLevels( const jobject& tagLevels )ι->void{
		auto parsedTags = parseTags<flat_map<ELogTags,ELogLevel>>( tagLevels );
		for( auto&& [tag, level] : parsedTags ){
			if( tag==ELogTags::None )
				_defaultLevel = level;
			else
				_configuredTags.insert_or_assign( tag, level );
		}
		ExtrapolatedTags = _configuredTags;
	}

	Ω split( ELogTags tags )ι->vector<ELogTags>{
		vector<ELogTags> result;
		for( uint i=0; i<ELogTagStrings.size(); ++i ){
			let flag = ( ELogTags )( 1ul<<(i-1) );
			if( !empty(tags & flag) )
				result.push_back( flag );
		}
		return result;
	}
	α LogTags::MinLevel( ELogTags tags )Ι->ELogLevel{
		optional<ELogLevel> level;
		if( ExtrapolatedTags.cvisit(tags, [&](let& kv){level = kv.second;}) )
			return *level;
		vector<ELogTags> individual = split( tags );
		if( individual.size()>1 ){
			uint matches{};
			ExtrapolatedTags.cvisit_while( [&level,&matches,tags,count=individual.size()](let& kv){
				if( empty(kv.first & tags) )
					return true;
				if( auto iterCount = split(tags).size(); iterCount>matches ){
					level = kv.second;
					matches = iterCount;
				}
				return matches+1<count;
			} );
		}
		let levelString = level ? Jde::ToString( *level ) : "unset";
		if( !level )
			level = _defaultLevel;
		ExtrapolatedTags.emplace( tags, *level );
		if( auto l = tags==ELogTags::Settings ? *level : _cumulative->MinLevel(ELogTags::Settings); l>=ELogLevel::Trace ){
			let name = Name();
			let tagString = Jde::ToString( tags );
			Logging::Log( ELogLevel::Trace, ELogTags::Settings, SRCE_CUR, "[{}]tag: {}, minLevel: {}", name, tagString, levelString );
		}
		return *level;
	}

	α LogTags::SetLevel( ELogTags tags, ELogLevel level )ι->void{
		std::cout << Ƒ( "[{}]tag: {}, minLevel: {}", Name(), Jde::ToString(tags), Jde::ToString(level) ) << std::endl;
		_configuredTags.insert_or_assign( tags, level );
		ExtrapolatedTags = _configuredTags;
		UpdateCumulative( Logging::Loggers() );
	}

	α LogTags::ShouldLog( ELogLevel level, ELogTags tags )Ι->bool{
		if( level==ELogLevel::NoLog )
			return false;
		let configuredMin = MinLevel( tags );
		return configuredMin!=ELogLevel::NoLog && configuredMin <= level;
	}

	α LogTags::ToString()ι->string{
		flat_map<ELogLevel, vector<string>> levels;
		_configuredTags.cvisit_all( [&](let& kv){
			levels.try_emplace( kv.second, vector<string>{} ).first->second.push_back( Jde::ToString(kv.first) );
		});
		string y; y.reserve( 1024 );
		for( auto& [level, tags] : levels )
			y += Ƒ( "{}: {}\n", FromEnum(LogLevelStrings(), level), Str::Join(tags, ",") );
		if( y.size() )
			y.pop_back();
		return y;
	}
}

α Jde::ShouldTrace( ELogTags tags )ι->bool {
	return _cumulative->MinLevel( tags ) == ELogLevel::Trace;
}
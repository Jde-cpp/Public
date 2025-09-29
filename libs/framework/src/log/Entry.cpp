#include <jde/framework/log/Entry.h>
#include <jde/framework/chrono.h>
#include <jde/framework/settings.h>
#include <jde/framework/str.h>
#include <jde/framework/io/crc.h>

namespace Jde{
	optional<ELogLevel> _breakLevel;
	α Logging::BreakLevel()ι->ELogLevel{
		if( !_breakLevel )
			_breakLevel = Settings::FindEnum<ELogLevel>( "/logging/breakLevel", ToLogLevel ).value_or( ELogLevel::Warning );
		return *_breakLevel;
	}
}
namespace Jde::Logging{
	Entry::Entry( SL sl, ELogLevel l, ELogTags tags, string&& m )ι:Entry{ sl, l, tags, move(m), vector<string>{} }{}
	Entry::Entry( SL sl, ELogLevel l, ELogTags tags, string&& m, vector<string> args )ι:
		Text{ move(m) },
		Arguments{ move(args) },
		Level{ l },
		Tags{ tags },
		Line{ sl.line() },
		Time{ Clock::now() },
		_fileName{ string{sl.file_name()} },
		_functionName{ string{sl.function_name()} }
	{}
	Entry::Entry( ELogLevel l, ELogTags tags, uint32_t line, TimePoint time, Jde::UserPK userId, uuid messageId, uuid fileId, uuid functionId, vector<string>&& args )ι:
		Arguments{ move(args) },
		Level{ l },
		Tags{ tags },
		Line{ line },
		Time{ time },
		UserPK{ userId },
		_id{ messageId },
		_fileId{ fileId },
		_functionId{ functionId }
	{}
	Entry::Entry( ELogLevel l, ELogTags tags, uint32_t line, TimePoint time, Jde::UserPK userId, string&& text, string&& file, string&& function, vector<string>&& args )ι:
		Text{ move(text) },
		Arguments{ move(args) },
		Level{ l },
		Tags{ tags },
		Line{ line },
		Time{ time },
		UserPK{ userId },
		_fileName{ move(file) },
		_functionName{ move(function) }
	{}

	function<StringMd5(sv)> _generateId;
	α Entry::SetGenerator( function<StringMd5(sv)> f )ι->void{ _generateId = f; }
	α Entry::GenerateId( sv text )ι->StringMd5{ return _generateId(text.substr(0,100)); }

	α Entry::SourceLocation()Ι->spdlog::source_loc{
		return spdlog::source_loc{
			_fileName.index()==0 ? std::get<sv>(_fileName).data() : std::get<string>(_fileName).c_str(),
			(int)Line,
			_functionName.index()==0 ? std::get<sv>(_functionName).data() : std::get<string>(_functionName).c_str()
		};
	}
	α Entry::Message()Ι->string{
		if( _message.size() )
			return _message;
    fmt::dynamic_format_arg_store<fmt::format_context> store;
		for( auto&& arg : Arguments ){
			store.push_back( arg );
		}
		try{
			_message = Arguments.size()==0 ? Text : fmt::vformat(Text, store);
		}
		catch( const exception& e ){
			CRITICALT( Tags, "Bad Format: {}", Text );
		}
    return _message;
	}
	α Entry::FileString()ι->string&{
		if( _fileName.index()==0 )
			_fileName = string{ get<sv>(_fileName) };
		return get<string>( _fileName );
	}
	α Entry::FunctionString()ι->string&{
		if( _functionName.index()==0 )
			_functionName = string{ get<sv>(_functionName) };
		return get<string>( _functionName );
	}
}


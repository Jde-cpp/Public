#pragma once
#ifndef ENTRY_H
#define ENTRY_H
#include <fmt/args.h>
#include <spdlog/common.h>
#include <jde/framework/log/logTags.h>
#include <jde/framework/utils/paramPack.h>

#define Φ Γ auto
#define let const auto
#define FormatString const fmt::format_string<Args const&...>
#define ARGS const Args&

namespace Jde{
	struct IException;
}
namespace Jde::Logging{
	struct Γ Entry final{
		template<class... Args> Entry( SL sl, ELogLevel l, ELogTags tags, string&& m, ARGS... args )ι;
		template<class... Args> Entry( const std::stacktrace_entry& sl, ELogLevel l, ELogTags tags, FormatString&& m, ARGS... args )ι;
		Entry( SL sl, ELogLevel l, ELogTags tags, string&& m )ι;
		Entry( SL sl, ELogLevel l, ELogTags tags, string&& m, vector<string> args )ι;
		Entry( ELogLevel l, ELogTags tags, uint32_t line, TimePoint time, Jde::UserPK userId, uuid messageId, uuid fileId, uuid functionId, vector<string>&& args )ι;
		Entry( ELogLevel l, ELogTags tags, uint32_t line, TimePoint time, Jde::UserPK userId, string&& text, string&& file, string&& function, vector<string>&& args )ι;

		Ω SetGenerator( function<StringMd5(sv)> f )ι->void;
		Ω GenerateId( sv text )ι->StringMd5;

		α Id()Ι->StringMd5{ if( !_id )_id = GenerateId(Text); return *_id; }
		α File()Ι->sv{ return _fileName.index()==0 ? std::get<sv>(_fileName) : std::get<string>(_fileName); }
		α FileString()ι->string&;
		α SetFile( sv file ){ _fileName = file; }
		α FileId()Ι->StringMd5{ return _fileId ? *_fileId : (_fileId = GenerateId(File())).value(); }
		α Function()Ι->sv{ return _functionName.index()==0 ? std::get<sv>(_functionName) : std::get<string>(_functionName); }
		α FunctionString()ι->string&;
		α SetFunction( sv function ){ _functionName = function; }
		α FunctionId()Ι->StringMd5{ return _functionId ? *_functionId : (_functionId = GenerateId(Function())).value(); }
		α Message()Ι->string;
		α SourceLocation()Ι->spdlog::source_loc;
		string Text; //template string with {} for args
		vector<string> Arguments;
		ELogLevel Level;
		ELogTags Tags;
		uint32_t Line;
		TimePoint Time;
		Jde::UserPK UserPK;
	private:
		variant<sv, string> _fileName;
		variant<sv, string> _functionName;
		mutable string _message;
		mutable optional<StringMd5> _id;
		mutable optional<StringMd5> _fileId;
		mutable optional<StringMd5> _functionId;
	};

	template<class... Args>
	Entry::Entry( SL sl, ELogLevel l, ELogTags tags, string&& m, ARGS... args )ι:
		Entry{ sl, l, tags, move(m), vector<string>{} }{
		ParamPack::Append( Arguments, args... );
	}

		template<class... Args>
	Entry::Entry( const std::stacktrace_entry& e, ELogLevel l, ELogTags tags, FormatString&& m, ARGS... args )ι:
		Text{ m.get().data(), m.get().size() },
		Level{ l },
		Tags{ tags },
		Line{ e.source_line() },
		Time{ Clock::now() },
		_message{ fmt::vformat(m, fmt::make_format_args(FWD(args)...)) },
		_fileName{ e.source_file() },
		_functionName{ e.description() }{
		ParamPack::Append( Arguments, FWD(args)... );
	}
}
#undef ARGS
#undef FormatString
#undef Φ
#undef let
#endif
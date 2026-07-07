#pragma once
#include <system_error>
#include "Exception.h"
#include "jde/fwk/log/logTags.h"

namespace Jde::IO{
#define CHECK_PATH( path, sl ) THROW_IFX( !fs::exists(path), IO::IOException(path, "path does not exist", sl) )
	struct Γ IOException final : Exception{
		IOException( fs::path path, uint32 code, string value, SRCE ):Exception{ move(value), {ELogLevel::Debug, ELogTags::IO, code}, sl }, _path{ move(path) }{ SetWhat(); }
		IOException( fs::path path, string value, SRCE ): Exception{ move(value), {ELogLevel::Debug, ELogTags::IO}, sl }, _path{ move(path) }{ SetWhat(); }
		IOException( fs::filesystem_error&& e, SRCE ):Exception{sl}, _underLying( mu<fs::filesystem_error>(move(e)) ){ SetWhat(); }
		template<class... Args> IOException( SL sl, const fs::path& path, ELogLevel level, fmt::format_string<Args...> m, Args&&... args ):Exception( sl, {level, ELogTags::IO}, m, std::forward<Args>(args)... ),_path{ path }{ SetWhat(); }

		α Path()Ι->const fs::path&; α SetPath( const fs::path& x )ι{ _path=x; }
		α what()const noexcept->const char* override;
		α Move()ι->up<Exception> override{ return mu<IOException>(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }
	private:
		α SetWhat()Ι->void;

		up<const fs::filesystem_error> _underLying;
		fs::path _path;
	};

	Ξ IOException::Path()Ι->const fs::path&{
		return  _underLying? _underLying->path1() : _path;
	}
	Ξ IOException::SetWhat()Ι->void{
		if( _underLying )
			_what = _underLying->what();
		else if( HasCode() ) //Code() is lazy & would mark HasCode - only touch it inside this branch.
			_what = Ƒ( "({}) {} - {} path='{}'", Code(), std::system_category().message((int)Code()), Exception::what(), Path().string() );
		else
			_what = Ƒ( "{} path='{}'", Exception::what(), Path().string() );
	}
	Ξ IOException::what()Ι->const char*{
		return _what.c_str();
	}
}
#pragma once
#ifndef JDE_EXCEPTION_H //gcc pragma once is not supported
#define JDE_EXCEPTION_H
#include "./Exports.h"
#include "io/Crc.h"
#include "collections/ToVec.h"

namespace boost::system{ class error_code; }

#define THROW(x, ...) throw Jde::Exception{ SRCE_CUR, Jde::ELogLevel::Error, x __VA_OPT__(,) __VA_ARGS__ }
#define IO_EX( path, level, msg, ... ) IOException( SRCE_CUR, path, level, msg __VA_OPT__(,) __VA_ARGS__ )
#define THROW_IF(condition, x, ...) if( condition ) THROW( x __VA_OPT__(,) __VA_ARGS__  )
#define THROW_IFSL(condition, x, ...) if( condition ) throw Jde::Exception{ sl, ELogLevel::Error, x __VA_OPT__(,) __VA_ARGS__ }
#define THROW_IFX(condition, x) if( condition ) throw x
#define THROW_IFL(condition, x, ...) if( condition ) throw Jde::Exception{ SRCE_CUR, ELogLevel::Error, x __VA_OPT__(,) __VA_ARGS__ }
#define CHECK(condition) if( !(condition) ) throw Jde::Exception{ SRCE_CUR, Jde::ELogLevel::Error, #condition }

#define RETHROW(x, ...) catch( std::exception& e ){ throw Exception{SRCE_CUR, move(e), x __VA_OPT__(,) __VA_ARGS__}; }
#define $ template<class... Args>
#define COMMON α Clone()ι->sp<IException> override{ return ms<T>(move(*this)); }\
	α Move()ι->up<IException> override{ return mu<T>(move(*this)); }\
	α Ptr()ι->std::exception_ptr override{ return Jde::make_exception_ptr(move(*this)); }\
	[[noreturn]] α Throw()->void override{ throw move(*this); }
namespace Jde{
	struct LogTag;
	Ξ make_exception_ptr( std::exception&& e )ι->std::exception_ptr{
		try{
			throw move(e);
		}
		catch (...){
			auto p = std::current_exception();
			return p;
		}
	}
	struct StackTrace{
		StackTrace( SL sl )ι{ stack.push_back(sl); }
		α front()Ι->SL&{ return stack.front(); }
		α size()Ι->uint{ return stack.size(); }
		vector<source_location> stack;
	};
	struct Γ IException : std::exception{
		using base=std::exception;
		IException( vector<string>&& args, string&& format, SL sl, uint c, ELogLevel l=DefaultLogLevel )ι;
		IException( string value, ELogLevel level=DefaultLogLevel, uint code=0, SRCE )ι;
		IException( string value, ELogLevel level=DefaultLogLevel, uint code=0, sp<LogTag>&& tag={}, SRCE )ι;
		IException( IException&& from )ι;
		IException( const IException& from )ι;

		$ IException( SL sl, std::exception&& inner, ELogLevel level, fmt::format_string<Args...> format_="", Args&&... args )ι;
		$ IException( SL sl, ELogLevel l, fmt::format_string<Args...> m, Args&& ...args )ι;
		$ IException( SL sl, ELogLevel l, uint code, fmt::format_string<Args...> m, Args&&... args )ι;
		Ω FromExceptionPtr( const std::exception_ptr& from, SRCE )ι->up<IException>;

		virtual ~IException();

		β Log()Ι->void;
		α what()Ι->const char* override;
		α What()Ι->const string&{ what(); return _what; }
		α PrependWhat( const string& prepend )ι->void{ _what = prepend+_what; }
		α Level()Ι->ELogLevel{return _level;} α SetLevel( ELogLevel level )Ι{ _level=level;}
		β Clone()ι->sp<IException> =0;
		β Move()ι->up<IException> =0;
		α Push( SL sl )ι{ _stack.stack.push_back(sl); }
		α Stack()Ι->const StackTrace&{ return _stack; }
		β Ptr()ι->std::exception_ptr =0;
		[[noreturn]] β Throw()->void=0;
	protected:
		IException( SRCE )ι:IException{ {}, ELogLevel::Debug, 0, sl }{}
		α BreakLog()Ι->void;
		StackTrace _stack;

		mutable string _what;
		sp<std::exception> _pInner;//sp to save custom copy constructor
		sv _format;
		sp<LogTag> _pTag;
		vector<string> _args;//TODO change to array
		static constexpr ELogLevel DefaultLogLevel{ ELogLevel::Debug };
	public:
		const uint Code; //after _format
	private:
		mutable ELogLevel _level;
	};
	struct Exception;
	α make_exception_ptr( Exception&& e )ι->std::exception_ptr;
	struct Γ Exception : IException{
		Exception( string what, ELogLevel l=ELogLevel::Debug, SRCE )ι;
		Exception( Exception&& from )ι:IException{ move(from) }{}
		Exception( const Exception& from )ι:IException{ from }{}
		Exception( string what, uint code, ELogLevel level=ELogLevel::Debug, SRCE )ι:IException{what, level, code, sl}{};
		$ Exception( SL sl, std::exception&& inner, ELogLevel level, fmt::format_string<Args...> m="", Args&&... args )ι:IException{sl, move(inner), level, m, std::forward<Args>(args)...}{}
		$ Exception( SL sl, std::exception&& inner, fmt::format_string<Args...> m="", Args&&... args )ι:Exception{sl, move(inner), DefaultLogLevel, m, std::forward<Args>(args)...}{}
		$ Exception( SL sl, ELogLevel l, fmt::format_string<Args...> fmt, Args&&... args )ι:IException( sl, l, fmt, std::forward<Args>(args)... ){}
		$ Exception( SL sl, fmt::format_string<Args...> m, Args&&... args )ι:IException( sl, DefaultLogLevel, m, std::forward<Args>(args)... ){}
		$ Exception( SL sl, ELogLevel l, uint code, fmt::format_string<Args...> fmt, Args&&... args )ι:IException( sl, l, code, fmt, std::forward<Args>(args)... ){}
		~Exception(){}
		using T=Exception;
		COMMON
	};

	Ξ make_exception_ptr( Exception&& e )ι->std::exception_ptr{
		try{
			throw move(e);
		}
		catch (...){
			auto p = std::current_exception();
			return p;
		}
	}

	struct Γ OSException final : IException{
#ifdef _MSC_VER
		using TErrorCode=DWORD;
#else
		using TErrorCode=_int;
#endif
		OSException( TErrorCode result, string&& msg, SRCE )ι;
		using T=OSException;
		COMMON
	};

	struct Γ CodeException final : IException{
		CodeException( std::error_code&& code, ELogLevel level=ELogLevel::Error, SRCE )ι;
		CodeException( std::error_code&& code, sp<LogTag> tag, ELogLevel level=ELogLevel::Error, SRCE )ι;
		CodeException( string value, std::error_code&& code, ELogLevel level=ELogLevel::Error, SRCE )ι;

		using T=CodeException;
		COMMON

		Ω ToString( const std::error_code& pErrorCode )ι->string;
		Ω ToString( const std::error_category& errorCategory )ι->string;
		Ω ToString( const std::error_condition& errorCondition )ι->string;
	private:
		std::error_code _errorCode;
	};

	//https://stackoverflow.com/questions/10176471/is-it-possible-to-convert-a-boostsystemerror-code-to-a-stderror-code
	struct Γ BoostCodeException final : IException{
		BoostCodeException( const boost::system::error_code& ec, sv msg={}, SRCE )ι;
		BoostCodeException( BoostCodeException&& e )ι;
		~BoostCodeException();

		using T=BoostCodeException;
		COMMON
	private:
		up<boost::system::error_code> _errorCode;
	};

#define CHECK_PATH( path, sl ) THROW_IFX( !fs::exists(path), IOException(path, "path does not exist", sl) );
	struct Γ IOException final : IException{
		IOException( fs::path path, uint code, string value, SRCE ):IException{ move(value), ELogLevel::Debug, code, sl }, _path{ move(path) }{ SetWhat(); }
		IOException( fs::path path, string value, SRCE ): IOException{ move(path), 0, move(value), sl }{}
		IOException( fs::filesystem_error&& e, SRCE ):IException{sl}, _pUnderLying( make_unique<fs::filesystem_error>(move(e)) ){ SetWhat(); }
		$ IOException( SL sl, const fs::path& path, ELogLevel level, fmt::format_string<Args...> m, Args&&... args ):IException( sl, level, m, std::forward<Args>(args)... ),_path{ path }{ SetWhat(); }

		α Path()Ι->const fs::path&; α SetPath( const fs::path& x )ι{ _path=x; }
		α what()Ι->const char* override;
		using T=IOException;
		COMMON
	private:
		α SetWhat()Ι->void;

		sp<const fs::filesystem_error> _pUnderLying;
		fs::path _path;
	};

	struct Γ NetException : IException{
		NetException( sv host, sv target, uint code, string result, ELogLevel level=ELogLevel::Debug, SRCE )ι;
		NetException( NetException&& f )ι:IException{ move(f) }, Host{ f.Host }, Target{ f.Target }, Result{ f.Result }{}
		~NetException(){ Log(); }
		α Log()Ι->void override{ Log( {} ); }
		α Log( string extra )Ι->void;

		using T=NetException;
		COMMON

		const string Host;
		const string Target;
		const string Result;
	};

#define var const auto
	$ IException::IException( SL sl, ELogLevel l, fmt::format_string<Args...> m, Args&&... args )ι:
		_stack{ sl },
		_format{ sv{m.get().data(), m.get().size()} },
		Code{ Calc32RunTime(_format) },
		_level{ l }{
		_args.reserve( sizeof...(args) );
		ToVec::Append( _args, args... );
		BreakLog();
	}

	$ IException::IException( SL sl, std::exception&& inner, ELogLevel l, fmt::format_string<Args...> m, Args&&... args )ι:
		_stack{ sl },
		_pInner{ make_shared<std::exception>(move(inner)) },
		_format{ sv{m.get().data(), m.get().size()} },
		Code{ Calc32RunTime(_format) },
		_level{ l }{
		_args.reserve( sizeof...(args) );
		ToVec::Append( _args, args... );
		BreakLog();
	}
	$ IException::IException( SL sl, ELogLevel l, uint code, fmt::format_string<Args...> m, Args&&... args )ι:
		_stack{ sl },
		_format{ sv{m.get().data(), m.get().size()} },
		//_format{ sv{spdlog::details::to_string_view(m).data(), spdlog::details::to_string_view(m).size()} },
		Code{ code },
		_level{ l }{
		_args.reserve( sizeof...(args) );
		ToVec::Append( _args, args... );
		BreakLog();
	}

	//https://stackoverflow.com/questions/35941045/can-i-obtain-c-type-names-in-a-constexpr-way/35943472#35943472
#ifdef _MSC_VER
	Ŧ constexpr GetTypeName()->sv{
#else
	Ŧ consteval GetTypeName()->sv{
#endif
#ifdef _MSC_VER
		char const* p = __FUNCSIG__;
#else
		char const* p = __PRETTY_FUNCTION__;
#endif
		while (*p++ != '=');
		for( ; *p == ' '; ++p );
		char const* p2 = p;
		int count = 1;
		for( ; ;++p2 ){
			switch( *p2 ){
			case '[':
				++count;
				break;
			case ']':
				--count;
				if (!count)
					return {p, std::size_t(p2 - p)};
			}
		}
	}
#define TRY(x) Try( [&]{x;} )
	Ξ Try( std::function<void()> func ){
		bool result = false;
		try{
			func();
			result = true;
		}
		catch( const IException& )
		{}
		return result;
	}
	Ŧ Try( std::function<T()> func ){
		optional<T> result;
		try{
			result = func();
		}
		catch( const IException& )
		{}
		return result;
	}

	template <class Y, class X> α cast( sp<X> x, sv error, SRCE )ε->sp<Y>{ sp<Y> y = std::dynamic_pointer_cast<X,Y>(x); if( !y ) throw Jde::Exception{sl, ELogLevel::Error, error}; return y; }

#undef $
#undef COMMON
#undef var
}
#endif
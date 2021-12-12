#pragma once

#include "./Exports.h"
#include "collections/ToVec.h"

namespace boost::system{ class error_code; }

#define THROW(x, ...) throw Jde::Exception{ SRCE_CUR, Jde::ELogLevel::Debug, x __VA_OPT__(,) __VA_ARGS__ }
#define IO_EX( p, v, ... ) IOException( SRCE_CUR, p, v __VA_OPT__(,) __VA_ARGS__ )
#define THROW_IF(condition, x, ...) if( condition ) THROW( x __VA_OPT__(,) __VA_ARGS__  )
#define THROW_IFSL(condition, x, ...) throw Jde::Exception{ sl, _logLevel.Level, x __VA_OPT__(,) __VA_ARGS__ }
#define THROW_IFX(condition, x) if( condition ) throw x
#define THROW_IFL(condition, x, ...) if( condition ) throw Jde::Exception{ SRCE_CUR, _logLevel.Level, x __VA_OPT__(,) __VA_ARGS__ }
#define CHECK(condition) THROW_IF( !(condition), #condition )

#define RETHROW(x, ...) catch( std::exception& e ){ throw Exception{SRCE_CUR, move(e), x __VA_OPT__(,) __VA_ARGS__}; }
#define $ template<class... Args>
#define COMMON α Ptr()->std::exception_ptr override{ return std::make_exception_ptr(*this); } [[noreturn]] α Throw()->void override{ throw *this; }
namespace Jde
{
	struct StackTrace
	{
		StackTrace( SL sl )noexcept{ stack.push_back(sl); }
		α front()const noexcept->SL&{ return stack.front(); }
		vector<source_location> stack;
	};
	struct Γ IException : std::exception
	{
		using base=std::exception;
		IException(SRCE)noexcept:_stack{sl}{};
		IException( vector<string>&& args, string&& format, const source_location& sl, ELogLevel l=ELogLevel::Debug )noexcept;
		IException( ELogLevel level, sv value, SRCE )noexcept;
		IException( string value, SRCE )noexcept;

		$ IException( const source_location& sl, std::exception&& inner, sv format_={}, Args&&... args )noexcept;
		$ IException( const source_location& sl, ELogLevel l, sv m, Args&& ...args )noexcept;

		virtual ~IException()=0;

		β Log()const noexcept->void;
		α what()const noexcept->const char* override;
		α Level()const noexcept->ELogLevel{return _level;}
		β Clone()noexcept->sp<IException> =0;
		α Push( const source_location& sl )noexcept{ _stack.stack.push_back(sl); }

		β Ptr()->std::exception_ptr =0;
		[[noreturn]] β Throw()->void=0;
	protected:
		StackTrace _stack;

		ELogLevel _level{ ELogLevel::Debug };
		mutable string _what;
		sp<std::exception> _pInner;//sp to save custom copy constructor
		sv _format;
		vector<string> _args;
	};

	struct Γ Exception : IException
	{
		Exception( sv what, ELogLevel l=ELogLevel::Debug, SRCE )noexcept;

		$ Exception( const source_location& sl, std::exception&& inner, sv format_={}, Args&&... args )noexcept:IException{sl, move(inner), format_, args...}{}
		$ Exception( const source_location& sl, ELogLevel l, sv format_, Args&&... args )noexcept:IException( sl, l, format_, args... ){}
		$ Exception( const source_location& sl, sv fmt, Args&&... args )noexcept:IException( sl, ELogLevel::Error, fmt, args... ){}
		~Exception(){}
		α Clone()noexcept->sp<IException> override{ return ms<Exception>(move(*this)); }
		COMMON
	};

	struct Γ OSException final : IException
	{
#ifdef _MSC_VER
		using T=DWORD;
#else
		using T=_int;
#endif
		OSException( T result, string&& msg, SRCE )noexcept;
		α Clone()noexcept->sp<IException> override{ return ms<OSException>(move(*this)); }
		COMMON
	};

	struct Γ CodeException final : IException
	{
		CodeException( std::error_code&& code, ELogLevel level=ELogLevel::Error, SRCE );
		CodeException( string value, std::error_code&& code, ELogLevel level=ELogLevel::Error, SRCE );

		α Clone()noexcept->sp<IException> override{ return ms<CodeException>(move(*this)); }
		COMMON

		Ω ToString( const std::error_code& pErrorCode )noexcept->string;
		Ω ToString( const std::error_category& errorCategory )noexcept->string;
		Ω ToString( const std::error_condition& errorCondition )noexcept->string;
	private:
		std::error_code _errorCode;
	};

	//https://stackoverflow.com/questions/10176471/is-it-possible-to-convert-a-boostsystemerror-code-to-a-stderror-code
	struct Γ BoostCodeException final : IException
	{
		BoostCodeException( const boost::system::error_code& ec, sv msg={}, SRCE )noexcept;
		BoostCodeException( const BoostCodeException& e )noexcept;
		~BoostCodeException();

		α Clone()noexcept->sp<IException> override
		{
			auto p = std::make_shared<BoostCodeException>(*this);
			return p;
		}
		COMMON
	private:
		up<boost::system::error_code> _errorCode;
	};

#define CHECK_PATH( path ) THROW_IFX( !fs::exists(path), IOException(path, "path does not exist") );
	struct Γ IOException final : IException
	{
		IOException( path path, string value, SRCE ): IException{ move(value), sl }, _path{path}{}
		IOException( path path, uint errorCode, string value, SRCE ):IException( move(value), sl ), _errorCode{errorCode}, _path{path}{}
		IOException( fs::filesystem_error&& e ):IException{}, _pUnderLying( make_unique<fs::filesystem_error>(move(e)) ){}
		$ IOException( const source_location& sl, path path, sv value, Args&&... args ):IException( sl, ELogLevel::Debug, value, args... ),_path{ path }{}

		α Clone()noexcept->sp<IException> override{ return ms<IOException>(move(*this)); }
		α ErrorCode()const noexcept->uint;
		α Path()const noexcept->path; α SetPath( path x )noexcept{ _path=x; }
		α what()const noexcept->const char* override;
		COMMON
	private:
		const uint _errorCode{ 0 };
		sp<const fs::filesystem_error> _pUnderLying;
		fs::path _path;
	};

	$ IException::IException( const source_location& sl, ELogLevel l, sv format_, Args&&... args )noexcept:
		_stack{ sl },
		_level{ l },
		_format{ format_ }
	{
		_args.reserve( sizeof...(args) );
		ToVec::Append( _args, args... );
	}

	$ IException::IException( const source_location& sl, std::exception&& inner, sv format_, Args&&... args )noexcept:
		_stack{ sl },
		_pInner{ make_shared<std::exception>(move(inner)) },
		_format{ format_ }
	{
		_args.reserve( sizeof...(args) );
		ToVec::Append( _args, args... );
	}

	//https://stackoverflow.com/questions/35941045/can-i-obtain-c-type-names-in-a-constexpr-way/35943472#35943472
#ifdef _MSC_VER
	ⓣ constexpr GetTypeName()->sv
#else
	ⓣ consteval GetTypeName()->sv
#endif
	{
#ifdef _MSC_VER
		char const* p = __FUNCSIG__;
#else
		char const* p = __PRETTY_FUNCTION__;
#endif
		while (*p++ != '=');
		for( ; *p == ' '; ++p );
		char const* p2 = p;
		int count = 1;
		for (;;++p2)
		{
			switch (*p2)
			{
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
	Ξ Try( std::function<void()> func )
	{
		bool result = false;
		try
		{
			func();
			result = true;
		}
		catch( const IException& )
		{}
		return result;
	}
	ⓣ Try( std::function<T()> func )
	{
		optional<T> result;
		try
		{
			result = func();
		}
		catch( const IException& )
		{}
		return result;
	}
#undef $
#undef COMMON
}
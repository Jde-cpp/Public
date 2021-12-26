#pragma once

#include "./Exports.h"
#include "io/Crc.h"
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
#define COMMON α Clone()noexcept->sp<IException> override{ return ms<T>(move(*this)); }\
	α Move()noexcept->up<IException> override{ return mu<T>(move(*this)); }\
	α Ptr()->std::exception_ptr override{ return Jde::make_exception_ptr(move(*this)); }\
	[[noreturn]] α Throw()->void override{ throw move(*this); }
namespace Jde
{
#ifdef _MSC_VER
	Ξ make_exception_ptr( std::exception&& e )noexcept->std::exception_ptr{ return std::make_exception_ptr(move(e)); }
#else
	Ξ make_exception_ptr( std::exception&& e )noexcept->std::exception_ptr{ try{ throw move(e); } catch (...){return std::current_exception();} }
#endif
	struct StackTrace
	{
		StackTrace( SL sl )noexcept{ stack.push_back(sl); }
		α front()const noexcept->SL&{ return stack.front(); }
		α size()const noexcept->uint{ return stack.size(); }
		vector<source_location> stack;
	};
	struct Γ IException : std::exception
	{
		using base=std::exception;
		IException( vector<string>&& args, string&& format, SL sl, uint c, ELogLevel l=ELogLevel::Debug )noexcept;
		IException( string value, ELogLevel level=ELogLevel::Debug, uint code=0, SRCE )noexcept;
		IException( IException&& from )noexcept;

		$ IException( SL sl, std::exception&& inner, sv format_={}, Args&&... args )noexcept;
		$ IException( SL sl, ELogLevel l, sv m, Args&& ...args )noexcept;

		virtual ~IException();

		β Log()const noexcept->void;
		α what()const noexcept->const char* override;
		α What()noexcept->string&&{ return move(_what); }
		α What()const noexcept->const string&{ return _what; }
		α Level()const noexcept->ELogLevel{return _level;}
		β Clone()noexcept->sp<IException> =0;
		β Move()noexcept->up<IException> =0;
		α Push( SL sl )noexcept{ _stack.stack.push_back(sl); }
		//β Id()const noexcept->uint32{ return _messageId; }
		β Ptr()->std::exception_ptr =0;
		[[noreturn]] β Throw()->void=0;
	protected:
		IException( SRCE )noexcept:IException{ {}, ELogLevel::Debug, 0, sl }{}

		StackTrace _stack;

		ELogLevel _level;
		mutable string _what;
		sp<std::exception> _pInner;//sp to save custom copy constructor
		sv _format;
		vector<string> _args;
	public:
		const uint Code;
	};

	struct Γ Exception : IException
	{
		Exception( string what, ELogLevel l=ELogLevel::Debug, SRCE )noexcept;
		Exception( Exception&& from ):IException{ move(from) }{}

		$ Exception( SL sl, std::exception&& inner, sv format_={}, Args&&... args )noexcept:IException{sl, move(inner), format_, args...}{}
		$ Exception( SL sl, ELogLevel l, sv format_, Args&&... args )noexcept:IException( sl, l, format_, args... ){}
		$ Exception( SL sl, sv fmt, Args&&... args )noexcept:IException( sl, ELogLevel::Error, fmt, args... ){}
		~Exception(){}
		using T=Exception;
		COMMON
	};

	struct Γ OSException final : IException
	{
#ifdef _MSC_VER
		using TErrorCode=DWORD;
#else
		using TErrorCode=_int;
#endif
		OSException( TErrorCode result, string&& msg, SRCE )noexcept;
		using T=OSException;
		COMMON
	};

	struct Γ CodeException final : IException
	{
		CodeException( std::error_code&& code, ELogLevel level=ELogLevel::Error, SRCE );
		CodeException( string value, std::error_code&& code, ELogLevel level=ELogLevel::Error, SRCE );

		using T=CodeException;
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
		BoostCodeException( BoostCodeException&& e )noexcept;
		~BoostCodeException();

		using T=BoostCodeException;
		COMMON
	private:
		up<boost::system::error_code> _errorCode;
	};

#define CHECK_PATH( path ) THROW_IFX( !fs::exists(path), IOException(path, "path does not exist") );
	struct Γ IOException final : IException
	{
		IOException( fs::path path, uint code, string value, SRCE ):IException{ move(value), ELogLevel::Debug, code, sl }, _path{ move(path) }{}
		IOException( fs::path path, string value, SRCE ): IOException{ move(path), 0, move(value), sl }{}
		IOException( fs::filesystem_error&& e, SRCE ):IException{sl}, _pUnderLying( make_unique<fs::filesystem_error>(move(e)) ){}
		$ IOException( SL sl, path path, sv value, Args&&... args ):IException( sl, ELogLevel::Debug, value, args... ),_path{ path }{}

		α Path()const noexcept->path; α SetPath( path x )noexcept{ _path=x; }
		α what()const noexcept->const char* override;
		using T=IOException;
		COMMON
	private:
		//const uint _errorCode{ 0 };
		sp<const fs::filesystem_error> _pUnderLying;
		fs::path _path;
	};

	$ IException::IException( SL sl, ELogLevel l, sv format_, Args&&... args )noexcept:
		_stack{ sl },
		_level{ l },
		_format{ format_ },
		Code{ Calc32RunTime(format_) }
	{
		_args.reserve( sizeof...(args) );
		ToVec::Append( _args, args... );
	}

	$ IException::IException( SL sl, std::exception&& inner, sv format_, Args&&... args )noexcept:
		_stack{ sl },
		_pInner{ make_shared<std::exception>(move(inner)) },
		_format{ format_ },
		Code{ Calc32RunTime(format_) }
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
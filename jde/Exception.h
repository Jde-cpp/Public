#pragma once

#include "./Exports.h"
#include "collections/ToVec.h"

namespace boost::system{ class error_code; }

//#ifdef THROW
	#undef THROW
//#endif
#define THROW(x, ...) throw Jde::Exception{ SRCE_CUR, x __VA_OPT__(,) __VA_ARGS__ }
//#endif
#define IO_EX( p, v, ... ) IOException( SRCE_CUR, p, v __VA_OPT__(,) __VA_ARGS__ )
//mysql undefs THROW :(
//#ifndef  THROW2
//# define THROW2(x, ...) throw Exception{ SRCE_CUR, x __VA_OPT__(,) __VA_ARGS__ }
//#endif
// #ifndef  THROWX
// # define THROWX(x) Jde::throw_exception(x, __func__,__FILE__,__LINE__)
// #endif

#define THROW_IF(condition, x, ...) if( condition ) THROW( x )
#define THROW_IFX2(condition, x) if( condition ) throw x
// #ifndef THROW_IFXSL
#define THROW_IFL(condition, x, ...) if( condition ) Jde::Exception{ SRCE_CUR, LogLevel(), x __VA_OPT__(,) __VA_ARGS__ }
// #endif
#ifndef  CHECK
# define CHECK(condition) THROW_IF( !(condition), #condition )
#endif
// #ifndef  LOG_EX
// # define LOG_EX(e) log_exception( e, __func__, __FILE__, __LINE__ )
// #endif

#define RETHROW(x, ...) catch( std::exception& e ){ throw Exception{SRCE_CUR, move(e), x __VA_OPT__(,) __VA_ARGS__}; }

namespace Jde
{
	struct Γ IException : std::exception
	{
		using base=std::exception;
		IException()noexcept=default;
		IException( vector<string>&& args, string&& format, const source_location& sl, ELogLevel l=ELogLevel::Debug )noexcept;
		IException( ELogLevel level, sv value, SRCE )noexcept;
		IException( sv value, SRCE )noexcept;

		template<class... Args> IException( const source_location& sl, std::exception&& inner, sv format_={}, Args&&... args )noexcept;
		template<class... Args> IException( const source_location& sl, ELogLevel l, sv m, Args&&... args )noexcept;
		template<class... Args> IException( const source_location& sl, sv m, Args&&... args )noexcept:IException{ sl, ELogLevel::Debug, m, ...args }{}
		virtual ~IException()=0;

		β Log()const noexcept->void;
		α what()const noexcept->const char* override;
		α Level()const noexcept->ELogLevel{return _level;}

		α SetFunction( const char* p )->void{ _sl = {_sl.file_name(), _sl.line(), p}; }
		α SetFile( const char* p )noexcept->void{ _sl = {p, _sl.line(), _sl.function_name()}; }
		α SetLine( uint_least32_t line )noexcept->void{ _sl = {_sl.file_name(), line, _sl.function_name()}; }
		//friend α operator<<( std::ostream& os, const Exception& e )->std::ostream&;
	protected:
		source_location _sl;
/*		sv _functionName;
		sv _fileName;
		uint_least32_t _line;*/

		ELogLevel _level{ ELogLevel::Debug };
		mutable string _what;
		sp<std::exception> _pInner;//sp to save custom copy constructor
		sv _format;
		vector<string> _args;
	};

	struct Γ Exception : IException
	{
		Exception( sv what, ELogLevel l=ELogLevel::Debug, SRCE )noexcept;

		template<class... Args> Exception( const source_location& sl, std::exception&& inner, sv format_={}, Args&&... args )noexcept:IException{sl, move(inner), format_, args...}{}
		template<class... Args> Exception( const source_location& sl, ELogLevel l, sv format_, Args&&... args )noexcept:IException( sl, l, format_, args... ){Log();}
		~Exception(){}
	};

	template<class... Args> IException::IException( const source_location& sl, ELogLevel l, sv format_, Args&&... args )noexcept:
		_sl{ sl },
		_level{ l },
		_format{ format_ }
	{
		_args.reserve( sizeof...(args) );
		ToVec::Append( _args, args... );
	}

	template<class... Args> IException::IException( const source_location& sl, std::exception&& inner, sv format_, Args&&... args )noexcept:
		_sl{ sl },
		_pInner{ make_shared<std::exception>(move(inner)) },
		_format{ format_ }
	{
		_args.reserve( sizeof...(args) );
		ToVec::Append( _args, args... );
	}

		//environment variables
/*	struct Γ EnvironmentException final : Exception
	{
		template<class... Args> EnvironmentException( const source_location& sl, sv value, Args&&... args ):
			Exception( value, args... )
		{
			_level = ELogLevel::Error;
		}
	};
*/
	struct Γ OSException final : IException
	{
#ifdef _MSC_VER
		using T=DWORD;
#else
		using T=_int;
#endif
		OSException( T result, string&& msg, SRCE )noexcept;
	private:
		//α Log()const noexcept->void;
		//const T _error;
		//const T _result;
	};

	struct Γ CodeException final : IException
	{
		CodeException( std::error_code&& code, ELogLevel level=ELogLevel::Error, SRCE );
		CodeException( sv value, std::error_code&& code, ELogLevel level=ELogLevel::Error, SRCE );

		//α what()const noexcept->const char* override;

		Ω ToString( const std::error_code& pErrorCode )noexcept->string;
		Ω ToString( const std::error_category& errorCategory )noexcept->string;
		Ω ToString( const std::error_condition& errorCondition )noexcept->string;
	private:
		//α Log()const noexcept->void;
		std::error_code _errorCode;
	};

	//https://stackoverflow.com/questions/10176471/is-it-possible-to-convert-a-boostsystemerror-code-to-a-stderror-code
	struct Γ BoostCodeException final : IException
	{
		BoostCodeException( const boost::system::error_code& ec, sv msg={}, SRCE )noexcept;
		BoostCodeException( const BoostCodeException& e, SRCE )noexcept;
		~BoostCodeException();
	private:
		//α Log()const noexcept->void;
		up<boost::system::error_code> _errorCode;
	};

/*
	struct Γ ArgumentException : Exception
	{
		template<class... Args>
		ArgumentException( sv value, Args&&... args ):
			Exception( value, args... )
		{}
	};
*/
#define CHECK_PATH( path ) THROW_IFX2( !fs::exists(path), IOException(path, "path does not exist") );
	struct Γ IOException final : IException
	{
		IOException( path path, sv value, SRCE ): IException( value, sl ), _path{path}{ Log(); }
		IOException( path path, uint errorCode, sv value, SRCE ):IException( value, sl ), _errorCode{errorCode}, _path{path}{ Log(); }

		template<class... Args> IOException( const source_location& sl, path path, sv value, Args&&... args ):
			IException( sl, value, args... ),
			_path{ path }
		{
			Log();
		}

/*		template<class... Args> IOException( const source_location& sl, path path, uint errorCode, sv value, Args&&... args ):
			IException( value, args... ),
			_errorCode{errorCode},
			_path{path}
		{
			Log();
		}*/
		/*
		template<class... Args>
		IOException( path path, sv value, Args&&... args ):
			IException( value, args... ),
			_path{path}
		{
			Log();
		}

		template<class... Args>
		IOException( uint errorCode, sv value, Args&&... args ):
			IException( value, args... ),
			_errorCode{errorCode}
		{
			Log();
		}
		*/
		IOException( fs::filesystem_error&& e ):
			IException{},
			_pUnderLying( make_unique<fs::filesystem_error>(move(e)) )
		{
			Log();
		}
		α ErrorCode()const noexcept->uint;
		α Path()const noexcept->path; α SetPath( path x )noexcept{ _path=x; }
		α Log()const noexcept->void override;
		α what()const noexcept->const char* override;
	private:
		const uint _errorCode{0};
		sp<const fs::filesystem_error> _pUnderLying;
		fs::path _path;
	};

	template<class T,std::enable_if_t<std::is_base_of<IException,T>::value>* = nullptr>
	void log_exception( T& e, const char* fnctn, const char* file, long line )noexcept(false)
	{
		e.SetFunction( fnctn );
		e.SetFile( file );
		e.SetLine( line );
		//e.Log();
	}
	template<class T,std::enable_if_t<std::is_base_of<IException,T>::value>* = nullptr>
/*	[[noreturn]] void throw_exception( T e, const char* fnctn, const char* file, long line )noexcept(false)
	{
		log_exception( e, fnctn, file, line );
		throw e;
	}*/
/*
	[[noreturn]] inline void throw_exception( sv what, const char* pszFunction, const char* pszFile, long line )
	{
		Exception e{ what };
		throw_exception<Exception>( e, pszFunction, pszFile, line );
	}
*/

	Γ void catch_exception( sv pszFunction, sv pszFile, long line, sv pszAdditional, const std::exception* pException=nullptr );
	//https://stackoverflow.com/questions/35941045/can-i-obtain-c-type-names-in-a-constexpr-way/35943472#35943472
	template<class T>
	constexpr sv GetTypeName()
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
	inline bool Try( std::function<void()> func )
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
	template<typename T>
	optional<T> Try( std::function<T()> func )
	{
		optional<T> result;
		try
		{
			result = func();
		}
		catch( const IException& )
		{
			//e.Log();
		}
		return result;
	}
}

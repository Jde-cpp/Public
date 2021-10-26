#pragma once

#include "./Exports.h"
#include "collections/ToVec.h"

namespace boost::system{ class error_code; }

#ifndef  THROW
# define THROW(x, ...) Jde::throw_exception(Jde::Exception(x __VA_OPT__(,) __VA_ARGS__), __func__,__FILE__,__LINE__)
#endif
//mysql undefs THROW :(
#ifndef  THROW2
# define THROW2(x, ...) Jde::throw_exception(Jde::Exception(x __VA_OPT__(,) __VA_ARGS__), __func__,__FILE__,__LINE__)
#endif
#ifndef  THROWX
# define THROWX(x) Jde::throw_exception(x, __func__,__FILE__,__LINE__)
#endif

#ifndef THROW_IF
# define THROW_IF(condition, x, ...) if( condition ) Jde::throw_exception( Jde::Exception(x __VA_OPT__(,) __VA_ARGS__), __func__, __FILE__, __LINE__ )
#endif
#ifndef THROW_IFX
# define THROW_IFX(condition, x) if( condition ) THROWX(x)
#endif
#ifndef THROW_IFXSL
	#define THROW_IFXSL(condition, x) if( condition ) Jde::throw_exception( x, sl.function_name(), sl.file_name(), sl.line() )
#endif
#ifndef  CHECK
# define CHECK(condition) THROW_IF( !(condition), #condition )
#endif
#ifndef  LOG_EX
# define LOG_EX(e) log_exception( e, __func__, __FILE__, __LINE__ )
#endif

namespace Jde
{
	struct Γ IException : std::exception
	{
		IException()noexcept=default;
		IException( vector<string>&& args, string&& format, const source_location& sl, ELogLevel l=ELogLevel::Debug )noexcept;
		IException( ELogLevel level, sv value, SRCE )noexcept;
		IException( sv value, SRCE )noexcept;

		template<class... Args> IException( IException&&, sv value, Args&&... args )noexcept;
		template<class... Args> IException( sv value, Args&&... args )noexcept;
		IException( const std::exception& exp )noexcept;
		virtual ~IException()=0;

		β Log()const noexcept->void;
		α what()const noexcept->const char* override;
		α Level()const noexcept->ELogLevel{return _level;}

		α SetFunction( const char* pszFunction )->void{ _functionName = {pszFunction, strlen(pszFunction)}; }
		α SetFile( const char* pszFile )noexcept->void{ _fileName = {pszFile, strlen(pszFile)}; }
		α SetLine( int line )noexcept->void{ _line = line; }
		//friend α operator<<( std::ostream& os, const Exception& e )->std::ostream&;
	protected:
		sv _functionName;
		sv _fileName;
		uint_least32_t _line;

		ELogLevel _level{ ELogLevel::Debug };
		mutable string _what;
		sp<IException> _pInner;//sp to save custom copy constructor
		string _format;
		vector<string> _args;
	};


	template<class... Args> IException::IException( sv value, Args&&... args )noexcept:
		_what{ fmt::vformat(value, fmt::make_format_args(std::forward<Args>(args)...)) },
		_format{ value }
	{
		_args.reserve( sizeof...(args) );
		ToVec::Append( _args, args... );
	}

	template<class... Args> IException::IException( IException&& e, sv value, Args&&... args )noexcept:
		IException{ value, args... }
	{
		_pInner = make_unique<IException>( e );
	}


	struct Γ Exception : IException
	{
		Exception( ELogLevel l, sv what, SRCE )noexcept;
		Exception( sv what, SRCE )noexcept:IException{what, sl}{Log();}

		template<class... Args>
		Exception( sv value, Args&&... args )noexcept:
			IException( value, args... )
		{
			Log();
		}
		//α Log()const noexcept->void;
	};


		//environment variables
	struct Γ EnvironmentException : Exception
	{
		template<class... Args>
		EnvironmentException( sv value, Args&&... args ):
			Exception( value, args... )
		{
			_level = ELogLevel::Error;
		}

	};
	
	struct Γ OSException : IException
	{
#ifdef _MSC_VER
		using T=DWORD;
#else
		using T=_int;
#endif
		OSException( T result, string&& msg, SRCE )noexcept;
	private:
		α Log()const noexcept->void;
		T _result;
		T _error;
	};

	struct Γ CodeException : IException
	{
		CodeException( std::error_code&& code, ELogLevel level=ELogLevel::Error );
		CodeException( sv value, std::error_code&& code, ELogLevel level=ELogLevel::Error );

		//α what()const noexcept->const char* override;

		Ω ToString( const std::error_code& pErrorCode )noexcept->string;
		Ω ToString( const std::error_category& errorCategory )noexcept->string;
		Ω ToString( const std::error_condition& errorCondition )noexcept->string;
	private:
		α Log()const noexcept->void;
		std::error_code _errorCode;
	};

	//https://stackoverflow.com/questions/10176471/is-it-possible-to-convert-a-boostsystemerror-code-to-a-stderror-code
	struct Γ BoostCodeException final : IException
	{
		BoostCodeException( const boost::system::error_code& ec, sv msg={} )noexcept;
		BoostCodeException( const BoostCodeException& e )noexcept;
		~BoostCodeException();
	private:
		α Log()const noexcept->void;
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
#define CHECK_FILE_EXISTS( path ) THROW_IFX( !fs::exists(path), IOException(path, "path does not exist") );
	struct Γ IOException : IException
	{
		IOException( path path, sv value, SRCE ):
			IException( value, sl ),
			_path{path}
		{
			Log();
		}

		template<class... Args> IOException( path path, sv value, Args&&... args ):
			IException( value, args... ),
			_path{ path }
		{
			Log();
		}
		
		template<class... Args> IOException( path path, uint errorCode, sv value, Args&&... args ):
			IException( value, args... ),
			_errorCode{errorCode},
			_path{path}
		{
			Log();
		}
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
		α Log()const noexcept->void;
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
	[[noreturn]] void throw_exception( T e, const char* fnctn, const char* file, long line )noexcept(false)
	{
		log_exception( e, fnctn, file, line );
		throw e;
	}
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

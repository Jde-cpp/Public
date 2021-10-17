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

#ifndef  THROW_IF
# define THROW_IF(condition, x, ...) if( condition ) Jde::throw_exception( Jde::Exception(x __VA_OPT__(,) __VA_ARGS__), __func__, __FILE__, __LINE__ )
#endif
#ifndef  THROW_IFX
# define THROW_IFX(condition, x) if( condition ) THROWX(x)
#endif
#ifndef  CHECK
# define CHECK(condition) THROW_IF( !(condition), #condition )
#endif
#ifndef  LOG_EX
# define LOG_EX(e) log_exception( e, __func__, __FILE__, __LINE__ )
#endif

namespace Jde
{
	struct JDE_NATIVE_VISIBILITY Exception : public std::exception
	{
		Exception()noexcept=default;
		//Exception( const Exception& )noexcept=default;
		Exception( ELogLevel level, sv value )noexcept;
		Exception( ELogLevel level, sv value, sv function, sv file, int line )noexcept;
		Exception( sv value )noexcept;

		template<class... Args>
		Exception( Exception&&, sv value, Args&&... args )noexcept;
		template<class... Args>
		Exception( sv value, Args&&... args )noexcept;
		Exception( const std::exception& exp )noexcept;
		virtual ~Exception();

		//Exception& operator=( const Exception& copyFrom );
		virtual void Log( sv pszAdditionalInformation="", optional<ELogLevel> level=nullopt )const noexcept;
		const char* what() const noexcept override;
		ELogLevel GetLevel()const{return _level;}

		void SetFunction( const char* pszFunction ){ _functionName = {pszFunction, strlen(pszFunction)}; }
		void SetFile( const char* pszFile ){ _fileName = {pszFile, strlen(pszFile)}; }
		void SetLine( int line ){ _line = line; }
		friend std::ostream& operator<<( std::ostream& os, const Exception& e );
	protected:
		sv _functionName;
		sv _fileName;
		int _line;

		ELogLevel _level{ ELogLevel::Debug };
		mutable string _what;
		sp<Exception> _pInner;//sp to save custom copy constructor
	private:
		string _format;
		vector<string> _args;
	};


	template<class... Args>
	Exception::Exception( sv value, Args&&... args )noexcept:
		_what{ fmt::vformat(value, fmt::make_format_args(std::forward<Args>(args)...)) },
		_format{ value }
	{
		_args.reserve( sizeof...(args) );
		ToVec::Append( _args, args... );
	}

	template<class... Args>
	Exception::Exception( Exception&& e, sv value, Args&&... args )noexcept:
		Exception{ value, args... }
	{
		_pInner = make_unique<Exception>( e );
	}


	//Before program runs
	struct JDE_NATIVE_VISIBILITY LogicException : public Exception
	{
		template<class... Args>
		LogicException( sv value, Args&&... args ):
			Exception( value, args... )
		{
			_level=ELogLevel::Critical;
		}
	};
	//environment variables
	struct JDE_NATIVE_VISIBILITY EnvironmentException : public Exception
	{
		template<class... Args>
		EnvironmentException( sv value, Args&&... args ):
			Exception( value, args... )
		{
			_level = ELogLevel::Error;
		}

	};


	//errors detectable when the program executes
	struct JDE_NATIVE_VISIBILITY RuntimeException : public Exception
	{
		RuntimeException()=default;
		RuntimeException( const std::runtime_error& inner );
		template<class... Args>
		RuntimeException( sv value, Args&&... args ):
			Exception( value, args... )
		{}
	};

	struct JDE_NATIVE_VISIBILITY CodeException : public RuntimeException
	{
		CodeException( std::error_code&& code, ELogLevel level=ELogLevel::Error );
		CodeException( sv value, std::error_code&& code, ELogLevel level=ELogLevel::Error );

		//α what()const noexcept->const char* override;

		Ω ToString( const std::error_code& pErrorCode )noexcept->string;
		Ω ToString( const std::error_category& errorCategory )noexcept->string;
		Ω ToString( const std::error_condition& errorCondition )noexcept->string;
	private:
		std::error_code _errorCode;
	};

	//https://stackoverflow.com/questions/10176471/is-it-possible-to-convert-a-boostsystemerror-code-to-a-stderror-code

	struct JDE_NATIVE_VISIBILITY BoostCodeException final : public RuntimeException
	{
		BoostCodeException( const boost::system::error_code& ec, sv msg={} )noexcept;
		BoostCodeException( const BoostCodeException& e )noexcept;
		~BoostCodeException();
	private:
		up<boost::system::error_code> _errorCode;
	};


	struct JDE_NATIVE_VISIBILITY ArgumentException : public RuntimeException
	{
		template<class... Args>
		ArgumentException( sv value, Args&&... args ):
			RuntimeException( value, args... )
		{}
	};

#define CHECK_FILE_EXISTS( path ) THROW_IF( !fs::exists(path), IOException{path, "path '{}' does not exist", path.string()} );
	struct JDE_NATIVE_VISIBILITY IOException : public RuntimeException
	{
		template<class... Args>
		IOException( sv value, Args&&... args ):
			RuntimeException( value, args... )
		{}
		template<class... Args>
		IOException( path path, sv value, Args&&... args ):
			RuntimeException( value, args... ),
			_path{path}
		{}
		template<class... Args>
		IOException( path path, uint errorCode, sv value, Args&&... args ):
			RuntimeException( value, args... ),
			_errorCode{errorCode},
			_path{path}
		{}

		template<class... Args>
		IOException( uint errorCode, sv value, Args&&... args ):
			RuntimeException( value, args... ),
			_errorCode{errorCode}
		{}

		IOException( fs::filesystem_error&& e ):
			RuntimeException{},
			_pUnderLying( make_unique<fs::filesystem_error>(move(e)) )
		{}


		uint ErrorCode()const noexcept;
		path Path()const noexcept; void SetPath( path x )noexcept{ _path=x; }

//		static void TestExists( path path, sv fileName )noexcept(false);
		const char* what() const noexcept override;
	private:
		const uint _errorCode{0};
		sp<const fs::filesystem_error> _pUnderLying;
		fs::path _path;
	};

	template<class T,std::enable_if_t<std::is_base_of<Exception,T>::value>* = nullptr>
	void log_exception( T& e, const char* fnctn, const char* file, long line )noexcept(false)
	{
		e.SetFunction( fnctn );
		e.SetFile( file );
		e.SetLine( line );
		e.Log();
	}
	template<class T,std::enable_if_t<std::is_base_of<Exception,T>::value>* = nullptr>
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

	JDE_NATIVE_VISIBILITY void catch_exception( sv pszFunction, sv pszFile, long line, sv pszAdditional, const std::exception* pException=nullptr );
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
		catch( const Exception& )
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
		catch( const Exception& e)
		{
			e.Log();
		}
		return result;
	}
}

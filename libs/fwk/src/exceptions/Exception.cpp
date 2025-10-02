#include <jde/fwk/exceptions/Exception.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <boost/system/error_code.hpp>

#include <jde/fwk/str.h>
#include <jde/fwk/exceptions/CodeException.h>
#include <jde/fwk/log/Entry.h>
#include <jde/fwk/log/SpdLog.h>
#define let const auto

namespace Jde{
	up<IException> _empty;
	α IException::EmptyPtr()ι->const up<IException>&{ return _empty; }

	IException::IException( string value, ELogLevel level, uint32 code, ELogTags tags, SL sl )ι:
		_stack{ sl },
		_format{ move(value) },
		_tags{ tags | ELogTags::Exception },
		Code( code ? code : Calc32RunTime(value) ),
		_level{ level }{
		BreakLog();
	}

	IException::IException( const IException& from )ι:
		_stack{ from._stack },
		_what{ from._what },
		_pInner{ from._pInner },
		_format{ from._format },
		_tags{ from._tags },
		_args{ from._args },
		Code{ from.Code },
		_level{ from.Level() }{
		BREAK;//should only be called by rethrow_exception
//		from._level = ELogLevel::NoLog;
	};
	IException::IException( IException&& from )ι:
		_stack{ move(from._stack) },
		_what{ move(from._what) },
		_pInner{ move(from._pInner) },
		_format{ move(from._format) },
		_tags{ from._tags },
		_args{ move(from._args) },
		Code{ from.Code },
		_level{ from.Level() }{
		BreakLog();
		ASSERT( _stack.stack.size() );
		from.SetLevel( ELogLevel::NoLog );
	}
	IException::~IException(){
		Log();
	}

	α IException::FromExceptionPtr( const std::exception_ptr& from, SL sl )ι->up<IException>{
		up<IException> y;
		try{
			std::rethrow_exception( from );
		}
		catch( IException& e ){
			y = e.Move();
		}
		catch( const std::exception& e ){
			y = mu<Exception>( Ƒ("std::exception - {}", e.what()), ELogLevel::Critical, sl );
		}
		catch( ... ){
			y = mu<Exception>( "unknown exception", ELogLevel::Critical, sl );
		}
		return y;
	}

	α IException::BreakLog()Ι->void{
#ifndef NDEBUG
		if( Level()>=Logging::BreakLevel() ){
			Log();
			SetLevel( ELogLevel::NoLog );
		}
#endif
	}
	α IException::Log()Ι->void{
		if( Level()==ELogLevel::NoLog || Process::Finalizing() )
			return;
		if( auto sv = Format(); sv.size() )
			Logging::Log( Logging::Entry{_stack.size() ? _stack.front() : SRCE_CUR, Level(), _tags, string{sv}, _args} );
		else
			Logging::Log( Logging::Entry{_stack.size() ? _stack.front() : SRCE_CUR, Level(), _tags, string{what()}} );
	}

	α IException::what()Ι->const char*{
		if( _what.empty() ){
			if( auto sv = Format(); sv.size() )
				_what = Str::Format( sv, _args );
			else if( _pInner )
				_what = _pInner->what();
		}
		return _what.c_str();
	}

	CodeException::CodeException( std::error_code code, ELogTags tags, ELogLevel level, SL sl )ι:
		ExternalException{ ToString(code), {}, (uint)code.value(), sl, tags, level },
		_errorCode{ move(code) }
	{}
	CodeException::CodeException( std::error_code code, ELogTags tags, string msg, ELogLevel level, SL sl )ι:
		ExternalException{ ToString(code), msg, (uint)code.value(), sl, tags, level },
		_errorCode{ move(code) }
	{}

	α CodeException::ToString( const std::error_code& errorCode )ι->string{
		let& category = errorCode.category();
		let message = errorCode.message();
		return Ƒ( "{} - {}", category.name(), message );
	}

	α CodeException::ToString( const std::error_category& errorCategory )ι->string{	return errorCategory.name(); }

	α CodeException::ToString( const std::error_condition& errorCondition )ι->string{
		const int value = errorCondition.value();
		const std::error_category& category = errorCondition.category();
		const string message = errorCondition.message();
		return Ƒ( "({}){} - {})", value, category.name(), message );
	}


	BoostCodeException::BoostCodeException( const boost::system::error_code& c, sv msg, SL sl )ι:
		IException{ string{msg}, ELogLevel::Debug, (uint32)c.value(), {}, sl },
		_errorCode{ c }
	{}
	BoostCodeException::BoostCodeException( BoostCodeException&& e )ι:
		IException{ move(e) },
		_errorCode{ move(e._errorCode) }
	{}
	BoostCodeException::~BoostCodeException()
	{}

	OSException::OSException( TErrorCode result, string&& m, SL sl )ι:
#ifdef _MSC_VER
		IException{ sl, ELogLevel::Error, (uint32)GetLastError(), "result={}/error={} - {}", result, GetLastError(), m }
#else
		IException{ sl, ELogLevel::Error, (uint)errno, "result={}/error={} - {}", result, errno, m }
#endif
	{}

	Exception::Exception( string what, ELogLevel l, SL sl )ι:
		IException{ move(what), l, 0, {}, sl }
	{}

	α IOException::Path()Ι->const fs::path&{
		return  _pUnderLying? _pUnderLying->path1() : _path;
	}
	α IOException::SetWhat()Ι->void{
#ifdef _MSC_VER
		let msg = std::strerror( Code );
#else
		let msg = std::strerror( errno );
#endif
		_what = _pUnderLying ? _pUnderLying->what() : Code
			? Ƒ( "({}) {} - {} path='{}'", Code, msg, IException::what(), Path().string() )
			: Ƒ( "({}){}", Path().string(), IException::what() );
	}
	α IOException::what()Ι->const char*{
		return _what.c_str();
	}
}
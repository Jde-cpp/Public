#pragma once
#include <stdexcept>
#include <jde/fwk/utils/paramPack.h>
#include "jde/fwk/log/logTags.h"
#include <jde/fwk/io/crc.h>

namespace Jde{
	static constexpr ELogLevel DefaultExceptionLevel{ ELogLevel::Debug };

	#define THROW(x, ...) throw Jde::Exception{ SRCE_CUR, {}, x __VA_OPT__(,) __VA_ARGS__ }
	#define THROWSL(x, ...) throw Jde::Exception{ sl, {}, x __VA_OPT__(,) __VA_ARGS__ }
	#define THROW_IF(condition, x, ...) if( condition ) THROW( x __VA_OPT__(,) __VA_ARGS__  )
	#define THROW_IFSL(condition, x, ...) if( condition ) throw Jde::Exception{ sl, {}, x __VA_OPT__(,) __VA_ARGS__ }
	#define THROW_IFX(condition, x) if( condition ) throw x
	#define CHECK(condition) if( !(condition) ) throw Jde::Exception( #condition, {Jde::ELogLevel::Error} )
	enum EHttpStatus : uint16{
		None = 0,
		Found = 302,
		BadRequest = 400,
		Unauthorized = 401,
		Forbidden = 403,
		NotFound = 404,
		Conflict = 409,
		IAmATeapot = 418,
		InternalServerError = 500,
		BadGateway = 502,
		ServiceUnavailable = 503,
		GatewayTimeout = 504
	};
	class ExceptionArgs{
	public:
		ExceptionArgs( const ExceptionArgs& args )ι:Tags{args.Tags}, _level{args._level}, _statusCode{args._statusCode}, _code{args._code}{}
		ExceptionArgs( ELogLevel level=DefaultExceptionLevel, ELogTags tags=ELogTags::Exception, uint32 code=UninitializedCode, EHttpStatus status=EHttpStatus::None )ι: Tags{tags}, _level{level}, _statusCode{status}, _code{code}{}
		ExceptionArgs( ELogTags tags, uint32 code=0 )ι:ExceptionArgs{DefaultExceptionLevel, tags, code}{}
		ExceptionArgs( uint32 code )ι:ExceptionArgs{DefaultExceptionLevel, ELogTags::Exception, code}{}
		ExceptionArgs( EHttpStatus status )ι:ExceptionArgs{DefaultExceptionLevel, ELogTags::Exception, UninitializedCode, status}{}

		α HasCode()Ι->bool{ return _code!=UninitializedCode; }
		α Level()Ι->ELogLevel{return _level;} α SetLevel( ELogLevel level )Ι{ _level=level;}

		ELogTags Tags;
	protected:
		mutable ELogLevel _level;
		EHttpStatus _statusCode{ EHttpStatus::None };
	private:
		static constexpr uint32 UninitializedCode{ (uint32)std::numeric_limits<int32_t>::max() };
	public:
		mutable uint32 _code;
	};
	#define $ template<class... Args>
	struct Γ Exception : std::runtime_error, ExceptionArgs{
		using base=std::runtime_error;

		Exception( Exception&& e )ι;
		Exception( const Exception& e )ι;
		Exception( runtime_error&& e, ExceptionArgs args={}, SRCE )ι;
		Ω FromPtr( const std::exception_ptr& e, SRCE )ι->up<Exception>; //preserves dynamic type, unlike a ctor which would slice.

		Exception( string value, ExceptionArgs args={}, SRCE )ι;
		$ Exception( SL sl, ExceptionArgs args, runtime_error&& inner, fmt::format_string<Args...> m="", Args&&... sargs )ι;
		$ Exception( SL sl, ExceptionArgs args, fmt::format_string<Args...> m, Args&&... sargs )ι;

		virtual ~Exception();

		β Log()Ι->void;
		//the part of this exception a client may see, empty unless a subclass opts in - what() carries internals. Response bodies append it, so a new funnel gets it without repeating the policy.
		β ClientDetail()Ι->string{ return {}; }
		enum class ECategory : uint8{ Jde, DB };//values mirror Common.proto ECategory - static_assert in app/shared/proto/common.cpp.
		//stored, not just virtual: a wire exception loses its concrete type, so the receiver's SetHttpStatus is all that keeps a 401 a 401.
		β HttpStatus()Ι->EHttpStatus{ return _statusCode ? _statusCode : EHttpStatus::InternalServerError; }
		α SetHttpStatus( EHttpStatus status )ι->void{ _statusCode = status; }
		β Category()Ι->ECategory{ return ECategory::Jde; }
		β CategoryCode()Ι->uint32{ return 0; }
		α what()const noexcept->const char* override;
		α What()Ι->const string&{ what(); return _what; }
		α PrependWhat( const string& prepend )ι->void{ What()/*initialize*/; _what = prepend+_what; }
		α Source()Ι->SL{ return _sl; }
		β Move()ι->up<Exception>{ return mu<Exception>(move(*this)); }
		[[noreturn]] β Throw()->void{ throw move(*this); }

		α SetTags( ELogTags tags )ι{ Tags = tags | ELogTags::Exception; }
		Ω EmptyPtr()ι->const up<Exception>&;
	protected:
		Exception( SRCE )ι:base{{}},_sl{ sl }{}
		α Format()Ι->sv{ return visit( []( auto&& arg )->sv{return {arg.data(),arg.size()};}, _format ); }
		α BreakLog()Ι->void;

		mutable string _what;
		mutable bool _logged{};//log once: at construction when Level()>=BreakLevel, otherwise at destruction/explicit Log().
		up<runtime_error> _inner;
		variant<sv,string> _format;
		vector<string> _args;
		SL _sl;
	private:
		α operator=( Exception&& from )ι->Exception&;
	public:
		α Code()Ι->uint32{
			if( !HasCode() )
				_code = Calc32RunTime( Format() );
			return _code;
		}
	};

	Ξ ToUP( runtime_error&& e )ι->up<runtime_error>{
		auto p = dynamic_cast<Exception*>( &e );
		return p ? p->Move() : up<runtime_error>{ mu<runtime_error>(e.what()) };//copying the runtime_error base would slice - its what() is the generic implementation string.
	}

	$ Exception::Exception( SL sl, ExceptionArgs args, runtime_error&& inner, fmt::format_string<Args...> m, Args&&... sargs )ι:
		runtime_error{ {} },
		ExceptionArgs{ args },
		_inner{ ToUP(move(inner)) }, //preserves derived message/type; mu<runtime_error> would slice.
		_format{ sv{m.get().data(), m.get().size()} },
		_sl{ sl }{
		_args.reserve( sizeof...(sargs) );
		ParamPack::Append( _args, sargs... );
		BreakLog();
	}

	$ Exception::Exception( SL sl, ExceptionArgs args, fmt::format_string<Args...> m, Args&&... sargs )ι:
		runtime_error{ {} },
		ExceptionArgs{ args },
		_format{ sv{m.get().data(), m.get().size()} },
		_sl{ sl }{
		_args.reserve( sizeof...(sargs) );
		ParamPack::Append( _args, sargs... );
		BreakLog();
	}

	[[noreturn]] Ξ Throw( runtime_error&& e )ε->void{
		if( auto p = dynamic_cast<Exception*>( &e ); p )
			p->Throw();
		else
			throw move(e);
	}
}
#undef $
#pragma once
#include <stdexcept>
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

	class ExceptionArgs{
	public:
		ExceptionArgs( const ExceptionArgs& args )ι:_level{args._level}, Tags{args.Tags}, _code{args._code}{}
		ExceptionArgs( ELogLevel level=DefaultExceptionLevel, ELogTags tags=ELogTags::Exception, uint32 code=UninitializedCode )ι: Tags{tags}, _level{level}, _code{code}{}
		ExceptionArgs( ELogTags tags )ι:ExceptionArgs{DefaultExceptionLevel, tags}{}
		ExceptionArgs( uint32 code )ι:ExceptionArgs{DefaultExceptionLevel, ELogTags::Exception, code}{}

		α HasCode()Ι->bool{ return _code!=UninitializedCode; }
		α Level()Ι->ELogLevel{return _level;} α SetLevel( ELogLevel level )Ι{ _level=level;}

		ELogTags Tags;
	protected:
		mutable ELogLevel _level;
	private:
		static constexpr uint32 UninitializedCode{ (uint32)std::numeric_limits<int32_t>::max() };
	public:
		mutable uint32 _code;
	};
	#define $ template<class... Args>
	struct Γ Exception : std::exception, ExceptionArgs{
		using base=std::exception;

		Exception( Exception&& e )ι;
		Exception( const Exception& e )ι;
		Exception( std::exception&& e, ExceptionArgs args={}, SRCE )ι;
		Ω FromPtr( const std::exception_ptr& e, SRCE )ι->up<Exception>; //preserves dynamic type, unlike a ctor which would slice.

		Exception( string value, ExceptionArgs args={}, SRCE )ι;
		$ Exception( SL sl, ExceptionArgs args, std::exception&& inner, fmt::format_string<Args...> m="", Args&&... sargs )ι;
		$ Exception( SL sl, ExceptionArgs args, fmt::format_string<Args...> m, Args&&... sargs )ι;

		virtual ~Exception();

		β Log()Ι->void;
		α what()const noexcept->const char* override;
		α What()Ι->const string&{ what(); return _what; }
		α PrependWhat( const string& prepend )ι->void{ What()/*initialize*/; _what = prepend+_what; }
		α Source()Ι->SL{ return _sl; }
		β Move()ι->up<Exception>{ return mu<Exception>(move(*this)); }
		[[noreturn]] β Throw()->void{ throw move(*this); }

		α SetTags( ELogTags tags )ι{ Tags = tags | ELogTags::Exception; }
		Ω EmptyPtr()ι->const up<Exception>&;
	protected:
		Exception( SRCE )ι:_sl{ sl }{}
		α Format()Ι->sv{ return visit( []( auto&& arg )->sv{return {arg.data(),arg.size()};}, _format ); }
		α BreakLog()Ι->void;

		mutable string _what;
		mutable bool _logged{};//log once: at construction when Level()>=BreakLevel, otherwise at destruction/explicit Log().
		up<std::exception> _inner;
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

	Ξ ToUP( exception&& e )ι->up<exception>{
		auto p = dynamic_cast<Exception*>( &e );
		return p ? p->Move() : up<exception>{ mu<std::runtime_error>(e.what()) };//copying the std::exception base would slice - its what() is the generic implementation string.
	}

	$ Exception::Exception( SL sl, ExceptionArgs args, std::exception&& inner, fmt::format_string<Args...> m, Args&&... sargs )ι:
		ExceptionArgs{ args },
		_inner{ ToUP(move(inner)) }, //preserves derived message/type; mu<std::exception> would slice.
		_format{ sv{m.get().data(), m.get().size()} },
		_sl{ sl }{
		_args.reserve( sizeof...(sargs) );
		ParamPack::Append( _args, sargs... );
		BreakLog();
	}

	$ Exception::Exception( SL sl, ExceptionArgs args, fmt::format_string<Args...> m, Args&&... sargs )ι:
		ExceptionArgs{ args },
		_format{ sv{m.get().data(), m.get().size()} },
		_sl{ sl }{
		_args.reserve( sizeof...(sargs) );
		ParamPack::Append( _args, sargs... );
		BreakLog();
	}

	[[noreturn]] Ξ Throw( exception&& e )ε->void{
		if( auto p = dynamic_cast<Exception*>( &e ); p )
			p->Throw();
		else
			throw move(e);
	}
}
#undef $
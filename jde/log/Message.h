#pragma once
#include "../db/usings.h"

namespace Jde::Logging{
	enum class EFields : uint16{ None=0, Timestamp=0x1, MessageId=0x2, Message=0x4, Level=0x8, FileId=0x10, File=0x20, FunctionId=0x40, Function=0x80, LineNumber=0x100, UserPK=0x200, User=0x400, ThreadId=0x800, Thread=0x1000, VariableCount=0x2000, SessionId=0x4000 };
	//TODO common enum operators
	constexpr inline EFields operator|(EFields a, EFields b){ return (EFields)( (uint16)a | (uint16)b ); }
	constexpr inline EFields operator&(EFields a, EFields b){ return (EFields)( (uint16)a & (uint16)b ); }
	constexpr inline EFields operator~(EFields a){ return (EFields)( ~(uint16)a ); }
	constexpr inline EFields& operator|=(EFields& a, EFields b){ return a = a | b; }
	//inline std::ostream& operator<<( std::ostream& os, const EFields& value ){ os << (uint)value; return os; }

	struct MessageBase{
		using ID=uint32;
		using ThreadID=uint;
		consteval MessageBase( sv message, ELogLevel level, const char* file, const char* function, uint_least32_t line, ID messageId=0, ID fileId=0, ID functionId=0 )ι;
		consteval MessageBase( sv message, const char* file, const char* function, uint_least32_t line )ι;
		consteval MessageBase( const char* file, const char* function, uint_least32_t line )ι;

		EFields Fields{ EFields::None };
		ELogLevel Level;
		ID MessageId{0};
		sv MessageView;
		ID FileId{0};
		const char* File;
		ID FunctionId{0};
		const char* Function;
		uint_least32_t LineNumber;
		Jde::UserPK UserPK{0};
		ThreadID ThreadId{0};
		Γ MessageBase( ELogLevel level, sv message, const char* file, const char* function, uint_least32_t line )ι;
		Γ MessageBase( ELogLevel level, ID messageId, ID fileId, ID functionId, uint_least32_t line, Jde::UserPK userPK, ThreadID threadId )ι;
	protected:
		explicit Γ MessageBase( ELogLevel level, SL sl )ι;
	};

	struct Message /*final*/ : MessageBase{
		Message( const MessageBase& b )ι;
		Message( const Message& x )ι;
		Γ Message( ELogLevel level, string message, SRCE )ι;
		Γ Message( sv Tag, ELogLevel level, string message, SRCE )ι;
		Γ Message( sv Tag, ELogLevel level, string message, char const* file_, char const * function_, boost::uint_least32_t line_ )ι;

		sv Tag;//TODO array
		up<string> _pMessage;//TODO move to protected
	protected:
		string _fileName;
	};

	consteval MessageBase::MessageBase( sv message, ELogLevel level, const char* file, const char* function, uint_least32_t line, ID messageId, ID fileId, ID functionId )ι:
		Level{level},
		MessageId{ messageId ? messageId : IO::Crc::Calc32(message.substr(0, 100)) },//{},
		MessageView{message},
		FileId{ fileId ? fileId : IO::Crc::Calc32(file) },
		File{file},
		FunctionId{ functionId ? functionId : IO::Crc::Calc32(function) },
		Function{function},
		LineNumber{line}{
		if( level!=ELogLevel::Trace )
			Fields |= EFields::Level;
		if( message.size() )
			Fields |= EFields::Message | EFields::MessageId;
		if( File[0]!='\0' )
			Fields |= EFields::File | EFields::FileId;
		if( Function[0]!='\0' )
			Fields |= EFields::Function | EFields::FunctionId;
		if( LineNumber )
			Fields |= EFields::LineNumber;
	}
	consteval MessageBase::MessageBase( sv message, const char* file, const char* function, uint_least32_t line )ι:
		MessageBase{ message, ELogLevel::Trace, file, function, line }
	{}

	consteval MessageBase::MessageBase( const char* file, const char* function, uint_least32_t line )ι:
		MessageBase( {}, file, function, line )
	{}
}
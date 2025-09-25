#pragma once
#ifndef UA_STRING
#define UA_STRING

namespace Jde::Opc{
	Ξ ToSV( UA_String s )ι->sv{ return sv{ (const char*)s.data, s.length }; }
	Ξ ToString( UA_String s )ι->string{ return string{ (const char*)s.data, s.length }; }
	Ξ ToUV( sv s )ι->UA_String{ return { s.size(), (UA_Byte*)s.data() }; }
	Ξ AllocUAString( str s )ι->UA_String{ return UA_String_fromChars( s.c_str() ); }
	Ξ AllocUAString( sv s )ι->UA_String{ return AllocUAString( string{s} ); }

	struct UAString final : UA_String, boost::noncopyable{
		explicit UAString( uint size )ι;
		~UAString()ι;

		α ToString()Ι->string{ return Opc::ToString( *this ); }
	};
}
#endif
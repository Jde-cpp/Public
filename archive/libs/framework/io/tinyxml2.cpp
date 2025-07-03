/*
Original code by Lee Thomason (www.grinninglizard.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#include <jde/framework/io/tinyxml2.h>

#include <new>		// yes, this one new style header, is in the Android SDK.
#include <cstddef>
#include <cstdarg>
#include <fstream>
#include <jde/framework/io/Parser.h>
#include <jde/framework/io/file.h>
#include <jde/framework/settings.h>

using namespace std::literals::string_view_literals;
#define let const auto
namespace Jde::Xml
{
	using namespace Str;
	template<class T=sv> α XmlTrim( bsv<typename T::traits_type> txt )ι->T;
}

namespace Jde{
	constexpr ELogTags _tags{ ELogTags::Parsing };
	using namespace Jde::Xml::XMLUtil;
	//const char* writeBoolTrue;
	//const char* writeBoolFalse;


#if defined(_MSC_VER) && (_MSC_VER >= 1400 ) && (!defined WINCE)
	static inline int TIXML_SNPRINTF( char* buffer, size_t size, const char* format, ... )
	{
		va_list va;
		va_start( va, format );
		const int result = vsnprintf_s( buffer, size, _TRUNCATE, format, va );
		va_end( va );
		return result;
	}

	static inline int TIXML_VSNPRINTF( char* buffer, size_t size, const char* format, va_list va )
	{
		const int result = vsnprintf_s( buffer, size, _TRUNCATE, format, va );
		return result;
	}

	#define TIXML_VSCPRINTF	_vscprintf
	#define TIXML_SSCANF	sscanf_s
#else
	// GCC version 3 and higher
	//#warning( "Using sn* functions." )
	#define TIXML_SNPRINTF	snprintf
	#define TIXML_VSNPRINTF	vsnprintf
/*	static inline int TIXML_VSCPRINTF( const char* format, va_list va )
	{
		int len = vsnprintf( 0, 0, format, va );
		TIXMLASSERT( len >= 0 );
		return len;
	}*/
	#define TIXML_SSCANF   sscanf
#endif

#if defined(_WIN64)
	#define TIXML_FSEEK _fseeki64
	#define TIXML_FTELL _ftelli64
#elif defined(__APPLE__) || defined(__FreeBSD__)
	#define TIXML_FSEEK fseeko
	#define TIXML_FTELL ftello
#elif defined(__unix__) && defined(__x86_64__)
	#define TIXML_FSEEK fseeko64
	#define TIXML_FTELL ftello64
#else
	#define TIXML_FSEEK fseek
	#define TIXML_FTELL ftell
#endif


static const char LINE_FEED				= static_cast<char>(0x0a);			// all line endings are normalized to LF
static const char LF = LINE_FEED;
static const char CARRIAGE_RETURN		= static_cast<char>(0x0d);			// CR gets filtered out
static const char CR = CARRIAGE_RETURN;
static const char SINGLE_QUOTE			= '\'';
static const char DOUBLE_QUOTE			= '\"';

// Bunch of unicode info at:
//		http://www.unicode.org/faq/utf_bom.html
//	ef bb bf (Microsoft "lead bytes") - designates UTF-8

static const unsigned char TIXML_UTF_LEAD_0 = 0xefU;
static const unsigned char TIXML_UTF_LEAD_1 = 0xbbU;
static const unsigned char TIXML_UTF_LEAD_2 = 0xbfU;
}

namespace Jde::Xml
{
	struct Entity{ sv Pattern; char value; };
	array<Entity,6> _entities{ Entity{"quot", DOUBLE_QUOTE},  Entity{"nbsp", ' ' }, Entity{ "amp", '&' }, Entity{ "apos", SINGLE_QUOTE }, Entity{ "lt", '<' }, Entity{ "gt", '>' } };

	StrPair::~StrPair()
	{
		Reset();
	}


// This in effect implements the assignment operator by "moving" ownership (as in auto_ptr).
void StrPair::TransferTo( StrPair* other )
{
	if( this == other )
		return;

	 TIXMLASSERT( other != 0 );
	 TIXMLASSERT( other->_flags == 0 );
	 TIXMLASSERT( other->_start == 0 );
	 TIXMLASSERT( other->_end == 0 );

	 other->Reset();

	 other->_flags = _flags;
	 other->_start = _start;
	 other->_end = _end;

	 _flags = 0;
	 _start = 0;
	 _end = 0;
}


void StrPair::Reset()
{
	 if ( _flags & NEEDS_DELETE ) {
		  delete [] _start;
	 }
	 _flags = 0;
	 _start = 0;
	 _end = 0;
}


void StrPair::SetStr( sv str, int flags )
{
	Reset();
	//size_t len = strlen( str );
	TIXMLASSERT( _start == 0 );
	_start = new char[ str.size()+1 ];
	memcpy( _start, str.data(), str.size() );
	_start[str.size()] = 0;
	_end = _start + str.size();
	_flags = flags | NEEDS_DELETE;
}


α StrPair::ParseText( char* p, const char* endTag, Mode mode, uint& line )->char*
{
	TIXMLASSERT( p );
	TIXMLASSERT( endTag && *endTag );

	let length = strlen( endTag );
	char* pResult = nullptr;
	for( char* start = p; *p && !pResult; ++p )
	{
		if( *p == *endTag && !strncmp(p, endTag, length) )
		{
			Set( start, p, mode );
			pResult = p + length;
		}
		else if( *p == '\n' )
			++line;
	}
	return pResult;
}


char* StrPair::ParseName( char* p )
{
	 if ( !p || !(*p) ) {
		  return 0;
	 }
	 if ( !IsNameStartChar( (unsigned char) *p ) ) {
		  return 0;
	 }

	 char* const start = p;
	 ++p;
	 while ( *p && IsNameChar( (unsigned char) *p ) ) {
		  ++p;
	 }

	 Set( start, p, 0 );
	 return p;
}


void StrPair::CollapseWhitespace()
{
	 // Adjusting _start would cause undefined behavior on delete[]
	 TIXMLASSERT( ( _flags & NEEDS_DELETE ) == 0 );
	 // Trim leading space.
	 _start = SkipWhiteSpace( _start, 0 );

	 if ( *_start ) {
		  const char* p = _start;	// the read pointer
		  char* q = _start;	// the write pointer

		  while( *p ) {
				if ( IsWhiteSpace( *p )) {
					 p = SkipWhiteSpace( p, 0 );
					 if ( *p == 0 ) {
						  break;    // don't write to q; this trims the trailing space.
					 }
					 *q = ' ';
					 ++q;
				}
				*q = *p;
				++q;
				++p;
		  }
		  *q = 0;
	 }
}


α StrPair::GetStr()->sv
{
	TIXMLASSERT( _start );
	TIXMLASSERT( _end );
	if ( _flags & NEEDS_FLUSH )
	{
		*_end = 0;
		_flags ^= NEEDS_FLUSH;
		if( _flags )
		{
			const char* p = _start;	// the read pointer
			char* q = _start;	// the write pointer
			while( p < _end )
			{
				if ( (_flags & NEEDS_NEWLINE_NORMALIZATION) && *p == CR ) // CR-LF pair becomes LF, CR alone becomes LF LF-CR becomes LF
				{
					if ( *(p+1) == LF )
						p += 2;
					else
						++p;
				  *q = LF;
				  ++q;
				}
				else if ( (_flags & NEEDS_NEWLINE_NORMALIZATION) && *p == LF )
				{
					if( *(p+1) == CR )
						p += 2;
					else
						++p;
					*q = LF;
					++q;
				}
				else if ( (_flags & NEEDS_ENTITY_PROCESSING) && *p == '&' )
				{
					if ( *(p+1) == '#' )
					{
						constexpr int buflen = 10;
						char buf[buflen] = { 0 };
						int len = 0;
						const char* adjusted = const_cast<char*>( GetCharacterRef( p, buf, &len ) );
						if ( adjusted == 0 )
						{
								*q = *p;
								++p;
								++q;
						}
						else
						{
							TIXMLASSERT( 0 <= len && len <= buflen );
							TIXMLASSERT( q + len <= adjusted );
							p = adjusted;
							memcpy( q, buf, len );
							q += len;
						}
					}
					else
					{
						bool entityFound = false;
						for( let& entity : _entities )
						{
							let size = entity.Pattern.size();
							if( entity.Pattern!=sv{(char*)p+1, size} || *(p+size+1)!=';' )
								continue;
							*q++ = entity.value;
							p += size + 2;
							entityFound = true;
							break;
						}
						if ( !entityFound )
						{
							++p;// fixme: treat as error?
							++q;
						}
					}
				}
				else
				{
					*q = *p;
					++p;
					++q;
				}
			}
			*q = 0;
		}
		if ( _flags & NEEDS_WHITESPACE_COLLAPSING )// The loop below has plenty going on, and this is a less useful mode. Break it out.
			CollapseWhitespace();
		_flags = (_flags & NEEDS_DELETE);
	}
	TIXMLASSERT( _start );
	return sv{ _start, std::min(strlen(_start), (Jde::uint)(_end-_start)) };
}


}
namespace Jde
{
	Ŧ Xml::XmlTrim( bsv<typename T::traits_type> txt )ι->T
	{
		return T{ Trim<T>(UnEscape<T>(txt)) }; static_assert( std::is_same<T, string>::value || std::is_same<T, String>::value, "can't be sv" );
	}

	//ELogLevel level{ ELogLevel::Trace };
	struct OpenTag{ uint Index; uint Line; String Tag; bool InnerText; };
	using Parser=IO::TokenParser<sv>;
	α Close( Parser parser, std::vector<OpenTag>& openTags )ε->string
	{
		string result; bool innerText{ false };
		for( auto token{parser.Next()}; token.size(); token=parser.Next(!innerText) )
		{
			//DEBUG_IF( parser.Line()==2414 );
			if( token=="begin"sv )
			{
				uint iStart{ parser.Index() }; uint line{ parser.Line() };
				let next = parser.Next(); iStart += next.find_first_of("\n")+1;
				if( let nextWord = Str::NextWord(next); nextWord=="644" )
				{
					for( sv n=parser.Next(false); n.size() && !n.ends_with("end"); n = parser.Next(false) );
					if( parser.Index()<parser.Text.size() )
					{
//						IO::FileUtilities::SaveBinary( "/tmp/new.xml", string{parser.Text.substr(0, iStart)}+string{parser.Text.substr(parser.Index())} );
						result = Close( Parser{string{parser.Text.substr(0, iStart)}+string{parser.Text.substr(parser.Index())}, &parser, iStart, line}, openTags );
					}
					break;
				}
				continue;
			}
			if( token.substr(token.size()-1,1)!="<" )
				continue;
			let next = parser.Next();
			auto tag = ToIV( Str::NextWord(next) ); CHECK( tag.size() );
			if( token=="<" && tag.starts_with("![CDATA[") && next.ends_with("]]>") )
				continue;
			if( tag.starts_with("!--") )
			{
				for( auto end=next; !end.ends_with("-->") && parser; end = parser.Next() );
				continue;
			}
			let haveClose = next.substr(next.size()-1, 1)==">";//<br > doesn't work with tag.
			if( tag.substr(tag.size()-1, 1)==">" )
				tag = tag.substr( 0, tag.size()-1 );
			if( tag.size() && tag[0]=='/' )// </html
			{
				let endTag = tag.substr( 1, tag.size()-1 ) ; CHECK( openTags.size() );
				let startTag = openTags.back();
				let equal = startTag.Tag==endTag;
				bool haveOpenTag = equal;
				for( size_t i=openTags.size(); !haveOpenTag && i>0; --i )
				{
					let& startTagCurrent = openTags[i-1].Tag;
					if( endTag=="div" && (startTagCurrent=="td" || startTagCurrent=="hr") )//<td></div></td>
						break;
					haveOpenTag = startTagCurrent==endTag;
				}
				if( !haveOpenTag )//<html></div></html>
				{
					for( size_t i=0; !equal && i<openTags.size(); ++i )
						Trace( _tags, "[{}][{}]{}", openTags[i].Line, openTags[i].Index, openTags[i].Tag );
					let endIndex = parser.Index()-tag.size()-2;
					let newXml = string{ parser.Text.substr(0, endIndex) }+Jde::format( "<{}>", ToSV(endTag) )+string{ parser.Text.substr(endIndex) };

					Trace( _tags, "[{}]({})noopentag = '{}|||{}", parser.Line(), ToSV(endTag), parser.Text.substr(parser.Index()-120, 120), parser.Text.substr(parser.Index(), 30) );
					Trace( _tags, "new = '{}", newXml.substr(parser.Index()-120, 150) );
					result = Close( Parser{newXml, &parser, parser.Index()}, openTags );
					break;
				}
				else
				{
					openTags.pop_back();
					innerText = openTags.size() && openTags.back().InnerText;
					if( !equal )
					{
						for( size_t i=0; !equal && i<openTags.size(); ++i )
							Trace( _tags, "[{}][{}]{}", openTags[i].Line, openTags[i].Index, openTags[i].Tag );
						let newXml = string{ parser.Text.substr(0, startTag.Index) }+string{"/"}+string{ parser.Text.substr(startTag.Index) };
						if( Settings::FindBool("/xml/closeLog").value_or(false) ){
							Trace( _tags, "({})start = [{}]'{}', end= [{}]'{}'", parser.Line(), startTag.Index, ToStr(startTag.Tag), parser.Index(), ToSV(endTag) );
							Trace( _tags, "old = '{}'", parser.Text.substr(startTag.Index-70, 100) );
							Trace( _tags, "new = '{}'", newXml.substr(startTag.Index-70, 100) );
						}
						parser.SetText( newXml, startTag.Index+2, startTag.Line );
					}
				}
			}
			else //<html
			{
				//DEBUG_IF( parser.Line()==139 );
				let closing = haveClose
					? next.size()<3 ? ">" : next[next.size()-2]=='/' ? "/>" : ">" //tag does not work with <br />
					: parser.NextToken( {"/>",">","<"} );
				if( !closing.ends_with("/>") )
				{
					if( closing.ends_with("<") ) //<font </p>  //<HR <P
					{
						let endIndex = parser.Index()-1;
						let newXml = string{ parser.Text.substr(0, parser.Index()-1) }+"/>"+string{ parser.Text.substr(endIndex) };

						Trace( _tags, "[{}]({})noclosetag = '{}|||{}", parser.Line(), ToSV(tag), parser.Text.substr(parser.Index()-120, 120), parser.Text.substr(parser.Index(), 30) );
						Trace( _tags, "new = '{}", newXml.substr(parser.Index()-120, 150) );
						result = Close( Parser{newXml, &parser, endIndex}, openTags );
						break;
					}
					else
					{
						ASSERT( closing.ends_with(">") );
						ASSERT( parser.Text[parser.Index()-1]=='>' );
						let i = parser.Text[parser.Index()]=='>' ? parser.Index() : parser.Index()-1;
						ASSERT( parser.Text[i]=='>' );
						if( tag=="hr" || tag=="br" ) //https://www.w3.org/TR/html401/index/elements.html
						{
							let newXml = string{ parser.Text.substr(0, i) }+string{"/"}+string{ parser.Text.substr(i) };
							Trace( _tags, "old = '{}'", parser.Text.substr(i-70, 100) );
							Trace( _tags, "new = '{}'", newXml.substr(i-70, 100) );
							ASSERT( newXml[i+1]=='>' );
							parser.SetText( newXml, i+1, parser.Line() );
						}
						else if( next.size() && next.ends_with("<") ) //<font <br, TODO, combine with 'if' above.
						{
							let start = i-next.size()-closing.size()+tag.size()+1;
							let newXml = string{ parser.Text.substr(0, start) }+string{"/>"}+string{ parser.Text.substr(start) };
							Trace( _tags, "old = '{}'", parser.Text.substr(i-70, 100) );
							Trace( _tags, "new = '{}'", newXml.substr(i-70, 100) );
							parser.SetText( newXml, i+1, parser.Line() );
						}
						else
						{
							openTags.push_back( {i, parser.Line(), String{tag}, innerText} );
							innerText = true;
						}
					}
				}
			}
		}
		return result.empty() ? string{ parser.Text } : result;
	}
	α Xml::Close( sv xml )ε->string
	{
		string result;
		vector<OpenTag> openTags;
		const vector<sv> tokens = { "<", "-->", "/>",">","=", "begin", "end" };
		return Jde::Close( Parser{xml, tokens}, openTags );
	}
}
namespace Jde::Xml
{
	namespace XMLUtil
	{
		const char* writeBoolTrue  = "true";
		const char* writeBoolFalse = "false";
	}
/*
void XMLUtil::SetBoolSerialization(const char* writeTrue, const char* writeFalse)
{
	static const char* defTrue  = "true";
	static const char* defFalse = "false";

	writeBoolTrue = (writeTrue) ? writeTrue : defTrue;
	writeBoolFalse = (writeFalse) ? writeFalse : defFalse;
}
*/

const char* XMLUtil::ReadBOM( const char* p, bool* bom )
{
	 TIXMLASSERT( p );
	 TIXMLASSERT( bom );
	 *bom = false;
	 const unsigned char* pu = reinterpret_cast<const unsigned char*>(p);
	 // Check for BOM:
	 if (    *(pu+0) == TIXML_UTF_LEAD_0
				&& *(pu+1) == TIXML_UTF_LEAD_1
				&& *(pu+2) == TIXML_UTF_LEAD_2 ) {
		  *bom = true;
		  p += 3;
	 }
	 TIXMLASSERT( p );
	 return p;
}


void XMLUtil::ConvertUTF32ToUTF8( unsigned long input, char* output, int* length )
{
	 const unsigned long BYTE_MASK = 0xBF;
	 const unsigned long BYTE_MARK = 0x80;
	 const unsigned long FIRST_BYTE_MARK[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

	 if (input < 0x80) {
		  *length = 1;
	 }
	 else if ( input < 0x800 ) {
		  *length = 2;
	 }
	 else if ( input < 0x10000 ) {
		  *length = 3;
	 }
	 else if ( input < 0x200000 ) {
		  *length = 4;
	 }
	 else {
		  *length = 0;    // This code won't convert this correctly anyway.
		  return;
	 }

	 output += *length;

	 // Scary scary fall throughs are annotated with carefully designed comments
	 // to suppress compiler warnings such as -Wimplicit-fallthrough in gcc
	 switch (*length) {
		  case 4:
				--output;
				*output = static_cast<char>((input | BYTE_MARK) & BYTE_MASK);
				input >>= 6;
				//fall through
		  case 3:
				--output;
				*output = static_cast<char>((input | BYTE_MARK) & BYTE_MASK);
				input >>= 6;
				//fall through
		  case 2:
				--output;
				*output = static_cast<char>((input | BYTE_MARK) & BYTE_MASK);
				input >>= 6;
				//fall through
		  case 1:
				--output;
				*output = static_cast<char>(input | FIRST_BYTE_MARK[*length]);
				break;
		  default:
				TIXMLASSERT( false );
	 }
}


const char* XMLUtil::GetCharacterRef( const char* p, char* value, int* length )
{
	 // Presume an entity, and pull it out.
	 *length = 0;

	 if ( *(p+1) == '#' && *(p+2) ) {
		  unsigned long ucs = 0;
		  TIXMLASSERT( sizeof( ucs ) >= 4 );
		  ptrdiff_t delta = 0;
		  unsigned mult = 1;
		  static const char SEMICOLON = ';';

		  if ( *(p+2) == 'x' ) {
				// Hexadecimal.
				const char* q = p+3;
				if ( !(*q) ) {
					 return 0;
				}

				q = strchr( q, SEMICOLON );

				if ( !q ) {
					 return 0;
				}
				TIXMLASSERT( *q == SEMICOLON );

				delta = q-p;
				--q;

				while ( *q != 'x' ) {
					 unsigned int digit = 0;

					 if ( *q >= '0' && *q <= '9' ) {
						  digit = *q - '0';
					 }
					 else if ( *q >= 'a' && *q <= 'f' ) {
						  digit = *q - 'a' + 10;
					 }
					 else if ( *q >= 'A' && *q <= 'F' ) {
						  digit = *q - 'A' + 10;
					 }
					 else {
						  return 0;
					 }
					 TIXMLASSERT( digit < 16 );
					 TIXMLASSERT( digit == 0 || mult <= UINT_MAX / digit );
					 const unsigned int digitScaled = mult * digit;
					 TIXMLASSERT( ucs <= ULONG_MAX - digitScaled );
					 ucs += digitScaled;
					 TIXMLASSERT( mult <= UINT_MAX / 16 );
					 mult *= 16;
					 --q;
				}
		  }
		  else {
				// Decimal.
				const char* q = p+2;
				if ( !(*q) ) {
					 return 0;
				}

				q = strchr( q, SEMICOLON );

				if ( !q ) {
					 return 0;
				}
				TIXMLASSERT( *q == SEMICOLON );

				delta = q-p;
				--q;

				while ( *q != '#' ) {
					 if ( *q >= '0' && *q <= '9' ) {
						  const unsigned int digit = *q - '0';
						  TIXMLASSERT( digit < 10 );
						  TIXMLASSERT( digit == 0 || mult <= UINT_MAX / digit );
						  const unsigned int digitScaled = mult * digit;
						  TIXMLASSERT( ucs <= ULONG_MAX - digitScaled );
						  ucs += digitScaled;
					 }
					 else {
						  return 0;
					 }
					 TIXMLASSERT( mult <= UINT_MAX / 10 );
					 mult *= 10;
					 --q;
				}
		  }
		  // convert the UCS to UTF-8
		  ConvertUTF32ToUTF8( ucs, value, length );
		  return p + delta + 1;
	 }
	 return p+1;
}


void XMLUtil::ToStr( int v, char* buffer, int bufferSize )
{
	 TIXML_SNPRINTF( buffer, bufferSize, "%d", v );
}

/*
void XMLUtil::ToStr( unsigned v, char* buffer, int bufferSize )
{
	 TIXML_SNPRINTF( buffer, bufferSize, "%u", v );
}


void XMLUtil::ToStr( bool v, char* buffer, int bufferSize )
{
	 TIXML_SNPRINTF( buffer, bufferSize, "%s", v ? writeBoolTrue : writeBoolFalse);
}

/ *
	ToStr() of a number is a very tricky topic.
	https://github.com/leethomason/tinyxml2/issues/106
* /
void XMLUtil::ToStr( float v, char* buffer, int bufferSize )
{
	 TIXML_SNPRINTF( buffer, bufferSize, "%.8g", v );
}


void XMLUtil::ToStr( double v, char* buffer, int bufferSize )
{
	 TIXML_SNPRINTF( buffer, bufferSize, "%.17g", v );
}


void XMLUtil::ToStr( int64_t v, char* buffer, int bufferSize )
{
	// horrible syntax trick to make the compiler happy about %lld
	TIXML_SNPRINTF(buffer, bufferSize, "%lld", static_cast<long long>(v));
}

void XMLUtil::ToStr( uint64_t v, char* buffer, int bufferSize )
{
	 // horrible syntax trick to make the compiler happy about %llu
	 TIXML_SNPRINTF(buffer, bufferSize, "%llu", (long long)v);
}*/
/*
bool XMLUtil::ToInt(const char* str, int* value)
{
	 if (IsPrefixHex(str)) {
		  unsigned v;
		  if (TIXML_SSCANF(str, "%x", &v) == 1) {
				*value = static_cast<int>(v);
				return true;
		  }
	 }
	 else {
		  if (TIXML_SSCANF(str, "%d", value) == 1) {
				return true;
		  }
	 }
	 return false;
}

bool XMLUtil::ToUnsigned(const char* str, unsigned* value)
{
	 if (TIXML_SSCANF(str, IsPrefixHex(str) ? "%x" : "%u", value) == 1) {
		  return true;
	 }
	 return false;
}

bool XMLUtil::ToBool( const char* str, bool* value )
{
	 int ival = 0;
	 if ( ToInt( str, &ival )) {
		  *value = (ival==0) ? false : true;
		  return true;
	 }
	 static const char* TRUE_VALS[] = { "true", "True", "TRUE", 0 };
	 static const char* FALSE_VALS[] = { "false", "False", "FALSE", 0 };

	 for (int i = 0; TRUE_VALS[i]; ++i) {
		  if (StringEqual(str, TRUE_VALS[i])) {
				*value = true;
				return true;
		  }
	 }
	 for (int i = 0; FALSE_VALS[i]; ++i) {
		  if (StringEqual(str, FALSE_VALS[i])) {
				*value = false;
				return true;
		  }
	 }
	 return false;
}


bool XMLUtil::ToFloat( const char* str, float* value )
{
	 if ( TIXML_SSCANF( str, "%f", value ) == 1 ) {
		  return true;
	 }
	 return false;
}


bool XMLUtil::ToDouble( const char* str, double* value )
{
	 if ( TIXML_SSCANF( str, "%lf", value ) == 1 ) {
		  return true;
	 }
	 return false;
}


bool XMLUtil::ToInt64(const char* str, int64_t* value)
{
	 if (IsPrefixHex(str)) {
		  unsigned long long v = 0;	// horrible syntax trick to make the compiler happy about %llx
		  if (TIXML_SSCANF(str, "%llx", &v) == 1) {
				*value = static_cast<int64_t>(v);
				return true;
		  }
	 }
	 else {
		  long long v = 0;	// horrible syntax trick to make the compiler happy about %lld
		  if (TIXML_SSCANF(str, "%lld", &v) == 1) {
				*value = static_cast<int64_t>(v);
				return true;
		  }
	 }
	return false;
}


bool XMLUtil::ToUnsigned64(const char* str, uint64_t* value) {
	 unsigned long long v = 0;	// horrible syntax trick to make the compiler happy about %llu
	 if(TIXML_SSCANF(str, IsPrefixHex(str) ? "%llx" : "%llu", &v) == 1) {
		  *value = (uint64_t)v;
		  return true;
	 }
	 return false;
}
*/

α XMLDocument::Identify( char* p, const XMLNode& parent )ι->tuple<char*,XMLNode*>
{
	 ASSERT( p );
	 char* const start = p;
	 let startLine = _parseCurLineNum;
	 if( WhitespaceMode()!=PRESERVE_WHITESPACE || parent.Value<iv>()!="P" )
		p = SkipWhiteSpace( p, &_parseCurLineNum );
	 if( !*p )
		return make_tuple( p, nullptr );

	 // These strings define the matching patterns:
	 static const char* xmlHeader		= { "<?" };
	 static const char* commentHeader	= { "<!--" };
	 static const char* cdataHeader		= { "<![CDATA[" };
	 static const char* dtdHeader		= { "<!" };
	 static const char* elementHeader	= { "<" };	// and a header for everything else; check last.

	 static const int xmlHeaderLen		= 2;
	 static const int commentHeaderLen	= 4;
	 static const int cdataHeaderLen		= 9;
	 static const int dtdHeaderLen		= 2;
	 static const int elementHeaderLen	= 1;

	 static_assert( sizeof( XMLComment ) == sizeof( XMLUnknown ) );		// use same memory pool
	 static_assert( sizeof( XMLComment ) == sizeof( XMLDeclaration ) );	// use same memory pool
	 XMLNode* returnNode = 0;
	 if ( StringEqual( p, xmlHeader, xmlHeaderLen ) ) {
		  returnNode = CreateUnlinkedNode<XMLDeclaration>( _commentPool );
		  returnNode->_parseLineNum = _parseCurLineNum;
		  p += xmlHeaderLen;
	 }
	 else if ( StringEqual( p, commentHeader, commentHeaderLen ) ) {
		  returnNode = CreateUnlinkedNode<XMLComment>( _commentPool );
		  returnNode->_parseLineNum = _parseCurLineNum;
		  p += commentHeaderLen;
	 }
	 else if ( StringEqual( p, cdataHeader, cdataHeaderLen ) ) {
		  XMLText* text = CreateUnlinkedNode<XMLText>( _textPool );
		  returnNode = text;
		  returnNode->_parseLineNum = _parseCurLineNum;
		  p += cdataHeaderLen;
		  text->SetCData( true );
	 }
	 else if ( StringEqual( p, dtdHeader, dtdHeaderLen ) ) {
		  returnNode = CreateUnlinkedNode<XMLUnknown>( _commentPool );
		  returnNode->_parseLineNum = _parseCurLineNum;
		  p += dtdHeaderLen;
	 }
	 else if ( StringEqual( p, elementHeader, elementHeaderLen ) ) {
		  returnNode =  CreateUnlinkedNode<XMLElement>( _elementPool );
		  returnNode->_parseLineNum = _parseCurLineNum;
		  p += elementHeaderLen;
	 }
	 else {
		  returnNode = CreateUnlinkedNode<XMLText>( _textPool );
		  returnNode->_parseLineNum = _parseCurLineNum; // Report line of first non-whitespace character
		  p = start;	// Back it up, all the text counts.
		  _parseCurLineNum = startLine;
	 }

	 ASSERT( returnNode ); ASSERT( p );
	 return make_tuple( p, returnNode );
}


bool XMLDocument::Accept( XMLVisitor* visitor ) const
{
	 TIXMLASSERT( visitor );
	 if ( visitor->VisitEnter( *this ) ) {
		  for ( const XMLNode* node=FirstChild(); node; node=node->NextSibling() ) {
				if ( !node->Accept( visitor ) ) {
					 break;
				}
		  }
	 }
	 return visitor->VisitExit( *this );
}


// --------- XMLNode ----------- //

XMLNode::XMLNode( XMLDocument* doc ) :
	 _document( doc ),
	 _parent( 0 ),
	 _value(),
	 _parseLineNum( 0 ),
	 _firstChild( 0 ), _lastChild( 0 ),
	 _prev( 0 ), _next( 0 ),
	_userData( 0 ),
	 _memPool( 0 )
{
}


XMLNode::~XMLNode()
{
	 DeleteChildren();
	 if ( _parent ) {
		  _parent->Unlink( this );
	 }
}
/*
const char* XMLNode::Value() const
{
	 // Edge case: XMLDocuments don't have a Value. Return null.
	 if ( this->ToDocument() )
		  return 0;
	 return _value.GetStr();
}
*/
void XMLNode::SetValue2( char* str )
{
	_value.SetInternedStr( str );
}
void XMLNode::SetValue( sv str/*, bool staticMem*/ )
{
/*	 if ( staticMem ) {
		  _value.SetInternedStr( str );
	 }
	 else {*/
		  _value.SetStr( str );
	 //}
}

XMLNode* XMLNode::DeepClone(XMLDocument* target) const
{
	XMLNode* clone = this->ShallowClone(target);
	if (!clone) return 0;

	for (const XMLNode* child = this->FirstChild(); child; child = child->NextSibling()) {
		XMLNode* childClone = child->DeepClone(target);
		TIXMLASSERT(childClone);
		clone->InsertEndChild(childClone);
	}
	return clone;
}

void XMLNode::DeleteChildren()
{
	 while( _firstChild ) {
		  TIXMLASSERT( _lastChild );
		  DeleteChild( _firstChild );
	 }
	 _firstChild = _lastChild = 0;
}


void XMLNode::Unlink( XMLNode* child )
{
	 TIXMLASSERT( child );
	 TIXMLASSERT( child->_document == _document );
	 TIXMLASSERT( child->_parent == this );
	 if ( child == _firstChild ) {
		  _firstChild = _firstChild->_next;
	 }
	 if ( child == _lastChild ) {
		  _lastChild = _lastChild->_prev;
	 }

	 if ( child->_prev ) {
		  child->_prev->_next = child->_next;
	 }
	 if ( child->_next ) {
		  child->_next->_prev = child->_prev;
	 }
	child->_next = 0;
	child->_prev = 0;
	child->_parent = 0;
}


void XMLNode::DeleteChild( XMLNode* node )
{
	 TIXMLASSERT( node );
	 TIXMLASSERT( node->_document == _document );
	 TIXMLASSERT( node->_parent == this );
	 Unlink( node );
	TIXMLASSERT(node->_prev == 0);
	TIXMLASSERT(node->_next == 0);
	TIXMLASSERT(node->_parent == 0);
	 DeleteNode( node );
}


XMLNode* XMLNode::InsertEndChild( XMLNode* addThis )
{
	 TIXMLASSERT( addThis );
	 if ( addThis->_document != _document ) {
		  TIXMLASSERT( false );
		  return 0;
	 }
	 InsertChildPreamble( addThis );

	 if ( _lastChild ) {
		  TIXMLASSERT( _firstChild );
		  TIXMLASSERT( _lastChild->_next == 0 );
		  _lastChild->_next = addThis;
		  addThis->_prev = _lastChild;
		  _lastChild = addThis;

		  addThis->_next = 0;
	 }
	 else {
		  TIXMLASSERT( _firstChild == 0 );
		  _firstChild = _lastChild = addThis;

		  addThis->_prev = 0;
		  addThis->_next = 0;
	 }
	 addThis->_parent = this;
	 return addThis;
}


XMLNode* XMLNode::InsertFirstChild( XMLNode* addThis )
{
	 TIXMLASSERT( addThis );
	 if ( addThis->_document != _document ) {
		  TIXMLASSERT( false );
		  return 0;
	 }
	 InsertChildPreamble( addThis );

	 if ( _firstChild ) {
		  TIXMLASSERT( _lastChild );
		  TIXMLASSERT( _firstChild->_prev == 0 );

		  _firstChild->_prev = addThis;
		  addThis->_next = _firstChild;
		  _firstChild = addThis;

		  addThis->_prev = 0;
	 }
	 else {
		  TIXMLASSERT( _lastChild == 0 );
		  _firstChild = _lastChild = addThis;

		  addThis->_prev = 0;
		  addThis->_next = 0;
	 }
	 addThis->_parent = this;
	 return addThis;
}


XMLNode* XMLNode::InsertAfterChild( XMLNode* afterThis, XMLNode* addThis )
{
	 TIXMLASSERT( addThis );
	 if ( addThis->_document != _document ) {
		  TIXMLASSERT( false );
		  return 0;
	 }

	 TIXMLASSERT( afterThis );

	 if ( afterThis->_parent != this ) {
		  TIXMLASSERT( false );
		  return 0;
	 }
	 if ( afterThis == addThis ) {
		  // Current state: BeforeThis -> AddThis -> OneAfterAddThis
		  // Now AddThis must disappear from it's location and then
		  // reappear between BeforeThis and OneAfterAddThis.
		  // So just leave it where it is.
		  return addThis;
	 }

	 if ( afterThis->_next == 0 ) {
		  // The last node or the only node.
		  return InsertEndChild( addThis );
	 }
	 InsertChildPreamble( addThis );
	 addThis->_prev = afterThis;
	 addThis->_next = afterThis->_next;
	 afterThis->_next->_prev = addThis;
	 afterThis->_next = addThis;
	 addThis->_parent = this;
	 return addThis;
}



α XMLNode::FirstChildElement( sv n )Ι->const XMLElement*
{
	let* y = FirstChild();
	for( ; y && (!y->ToElement() || (!n.empty() && !StringEqual(y->Name(),n,IsCaseInsensitive()))); y = y->NextSiblingElement(n) );

	return y && y->ToElement() ? y->ToElement() : nullptr;
}
/*
const XMLElement* XMLNode::FirstChildElement( const char* name ) const
{
	 for( const XMLNode* node = _firstChild; node; node = node->_next ) {
		  const XMLElement* element = node->ToElementWithName( name );
		  if ( element ) {
				return element;
		  }
	 }
	 return 0;
}
*/

const XMLElement* XMLNode::LastChildElement( const char* name ) const
{
	 for( const XMLNode* node = _lastChild; node; node = node->_prev ) {
		  const XMLElement* element = node->ToElementWithName( name );
		  if ( element ) {
				return element;
		  }
	 }
	 return 0;
}

/*
const XMLElement* XMLNode::NextSiblingElement( const char* name ) const
{
	 for( const XMLNode* node = _next; node; node = node->_next ) {
		  const XMLElement* element = node->ToElementWithName( name );
		  if ( element ) {
				return element;
		  }
	 }
	 return 0;
}
*/
α XMLNode::NextSiblingElement( sv name )Ι->const XMLElement*
{
	const XMLElement* y = nullptr;
	for( auto p = _next; p && !y; p = p->_next )
		y = p->ToElementWithName( name );

	return y;
}

const XMLElement* XMLNode::PreviousSiblingElement( const char* name ) const
{
	 for( const XMLNode* node = _prev; node; node = node->_prev ) {
		  const XMLElement* element = node->ToElementWithName( name );
		  if ( element ) {
				return element;
		  }
	 }
	 return 0;
}


char* XMLNode::ParseDeep( char* p, StrPair* parentEndTag, uint* curLineNumPtr )
{
	 // This is a recursive method, but thinking about it "at the current level"
	 // it is a pretty simple flat list:
	 //		<foo/>
	 //		<!-- comment -->
	 //
	 // With a special case:
	 //		<foo>
	 //		</foo>
	 //		<!-- comment -->
	 //
	 // Where the closing element (/foo) *must* be the next thing after the opening
	 // element, and the names must match. BUT the tricky bit is that the closing
	 // element will be read by the child.
	 //
	 // 'endTag' is the end tag for this node, it is returned by a call to a child.
	 // 'parentEnd' is the end tag for the parent, which is filled in and returned.
	XMLDocument::DepthTracker _( _document );
	if( _document->Error() )
		return 0;
	uint line{0};
	while( p && *p )
	{
		XMLNode* node;
		//DEBUG_IF( _document->Line()==138 );
		std::tie(p,node) = _document->Identify( p, *this );
		if( !node )
			break;
		let initialLineNum = line = node->_parseLineNum;

		StrPair endTag;
		//DEBUG_IF( initialLineNum==138 );
		p = node->ParseDeep( p, &endTag, curLineNumPtr );
		if ( !p )
		{
			BREAK;
			DeleteNode( node );
			if( !_document->Error() )
				_document->SetError( XML_ERROR_PARSING, initialLineNum, 0);
			break;
		}
		const XMLDeclaration* const decl = node->ToDeclaration();
		  if ( decl ) {
				// Declarations are only allowed at document level
				//
				// Multiple declarations are allowed but all declarations
				// must occur before anything else.
				//
				// Optimized due to a security test case. If the first node is
				// a declaration, and the last node is a declaration, then only
				// declarations have so far been added.
				bool wellLocated = false;

				if (ToDocument()) {
					 if (FirstChild()) {
						  wellLocated =
								FirstChild() &&
								FirstChild()->ToDeclaration() &&
								LastChild() &&
								LastChild()->ToDeclaration();
					 }
					 else {
						  wellLocated = true;
					 }
				}
				if ( !wellLocated ) {
					 _document->SetError( XML_ERROR_PARSING_DECLARATION, initialLineNum, "XMLDeclaration value=%s", decl->Value());
					 DeleteNode( node );
					 break;
				}
		  }

		  XMLElement* ele = node->ToElement();
		  if ( ele ) {
				// We read the end tag. Return it to the parent.
				if ( /*autoClose ||*/ ele->ClosingType() == XMLElement::CLOSING ) {
					 if ( parentEndTag ) {
						  ele->_value.TransferTo( parentEndTag );
					 }
					 node->_memPool->SetTracked();   // created and then immediately deleted.
					 DeleteNode( node );
					 return p;
				}
				try
				{
					if( endTag.Empty() )
					{
						THROW_IFX( ele->ClosingType() == XMLElement::OPEN, Jde::Exception(SRCE_CUR, Jde::ELogLevel::Error, "ele->ClosingType() == XMLElement::OPEN") );
					}
					else
					{
						THROW_IFX( ele->ClosingType() != XMLElement::OPEN, Jde::Exception(SRCE_CUR, Jde::ELogLevel::Error, "(<{}>)line:  {}, ele->ClosingType():{}!=XMLElement::OPEN", ele->Name(), initialLineNum, XMLElement::ToString(ele->ClosingType())) );
						THROW_IFX( !StringEqual(endTag.GetStr(), ele->Name(), IsCaseInsensitive()), Jde::Exception(SRCE_CUR, Jde::ELogLevel::Error, "line:  {} endTag:'{}'!=element:  '{}'", initialLineNum, endTag.GetStr(), ele->Name()) );
					}
				}
				catch( const IException& )
				{
					_document->SetError( XML_ERROR_MISMATCHED_ELEMENT, initialLineNum, "XMLElement name=%s", ele->Name() );
					DeleteNode( node );
					break;
				}
		  }
		  InsertEndChild( node );
	 }
	 return 0;
}

/*static*/ void XMLNode::DeleteNode( XMLNode* node )
{
	 if ( node == 0 ) {
		  return;
	 }
	TIXMLASSERT(node->_document);
	if (!node->ToDocument()) {
		node->_document->MarkInUse(node);
	}

	 MemPool* pool = node->_memPool;
	 node->~XMLNode();
	 pool->Free( node );
}

void XMLNode::InsertChildPreamble( XMLNode* insertThis ) const
{
	 TIXMLASSERT( insertThis );
	 TIXMLASSERT( insertThis->_document == _document );

	if (insertThis->_parent) {
		  insertThis->_parent->Unlink( insertThis );
	}
	else {
		insertThis->_document->MarkInUse(insertThis);
		  insertThis->_memPool->SetTracked();
	}
}

const XMLElement* XMLNode::ToElementWithName( const char* name ) const
{
	 const XMLElement* element = this->ToElement();
	 if ( element == 0 ) {
		  return 0;
	 }
	 if ( name == 0 ) {
		  return element;
	 }
	 if ( StringEqual( element->Name(), name, IsCaseInsensitive() ) ) {
		 return element;
	 }
	 return 0;
}

α XMLNode::ToElementWithName( sv n )const->const XMLElement*
{
	 let p = this->ToElement();
	 return n.empty() || (p && StringEqual(p->Name(), n, IsCaseInsensitive())) ? p : nullptr;
}

// --------- XMLText ---------- //
char* XMLText::ParseDeep( char* p, StrPair*, uint* curLineNumPtr )
{
	 if ( this->CData() ) {
		  p = _value.ParseText( p, "]]>", StrPair::NEEDS_NEWLINE_NORMALIZATION, *curLineNumPtr );
		  if ( !p )
				_document->SetError( XML_ERROR_PARSING_CDATA, _parseLineNum, 0 );
		  return p;
	 }
	 else
	 {
		 StrPair::Mode flags = _document->ProcessEntities() ? StrPair::TEXT_ELEMENT : StrPair::TEXT_ELEMENT_LEAVE_ENTITIES;
		  if ( _document->WhitespaceMode() == COLLAPSE_WHITESPACE )
				flags |= StrPair::NEEDS_WHITESPACE_COLLAPSING;

		  p = _value.ParseText( p, "<", flags, *curLineNumPtr );
		  if ( p && *p )
				return p-1;
		  if ( !p )
				_document->SetError( XML_ERROR_PARSING_TEXT, _parseLineNum, 0 );
	 }
	 return 0;
}


XMLNode* XMLText::ShallowClone( XMLDocument* doc ) const
{
	 if ( !doc ) {
		  doc = _document;
	 }
	 XMLText* text = doc->NewText( Value() );	// fixme: this will always allocate memory. Intern?
	 text->SetCData( this->CData() );
	 return text;
}


bool XMLText::ShallowEqual( const XMLNode* compare ) const
{
	 TIXMLASSERT( compare );
	 const XMLText* text = compare->ToText();
	 return text && StringEqual( text->Value(), Value(), IsCaseInsensitive() );
}


bool XMLText::Accept( XMLVisitor* visitor ) const
{
	 TIXMLASSERT( visitor );
	 return visitor->Visit( *this );
}


// --------- XMLComment ---------- //

XMLComment::XMLComment( XMLDocument* doc ) : XMLNode( doc )
{
}


XMLComment::~XMLComment()
{
}


char* XMLComment::ParseDeep( char* p, StrPair*, uint* curLineNumPtr )
{
	 // Comment parses as text.
	 p = _value.ParseText( p, "-->", StrPair::COMMENT, *curLineNumPtr );
	 if ( p == 0 ) {
		  _document->SetError( XML_ERROR_PARSING_COMMENT, _parseLineNum, 0 );
	 }
	 return p;
}


XMLNode* XMLComment::ShallowClone( XMLDocument* doc ) const
{
	 if ( !doc ) {
		  doc = _document;
	 }
	 XMLComment* comment = doc->NewComment( Value() );	// fixme: this will always allocate memory. Intern?
	 return comment;
}


bool XMLComment::ShallowEqual( const XMLNode* compare ) const
{
	 TIXMLASSERT( compare );
	 const XMLComment* comment = compare->ToComment();
	 return comment && StringEqual( comment->Value(), Value(), IsCaseInsensitive() );
}


bool XMLComment::Accept( XMLVisitor* visitor ) const
{
	 TIXMLASSERT( visitor );
	 return visitor->Visit( *this );
}


// --------- XMLDeclaration ---------- //

XMLDeclaration::XMLDeclaration( XMLDocument* doc ) : XMLNode( doc )
{
}


XMLDeclaration::~XMLDeclaration()
{
	 //printf( "~XMLDeclaration\n" );
}


char* XMLDeclaration::ParseDeep( char* p, StrPair*, uint* curLineNumPtr )
{
	 // Declaration parses as text.
	 p = _value.ParseText( p, "?>", StrPair::NEEDS_NEWLINE_NORMALIZATION, *curLineNumPtr );
	 if ( p == 0 ) {
		  _document->SetError( XML_ERROR_PARSING_DECLARATION, _parseLineNum, 0 );
	 }
	 return p;
}


XMLNode* XMLDeclaration::ShallowClone( XMLDocument* doc ) const
{
	 if ( !doc ) {
		  doc = _document;
	 }
	 XMLDeclaration* dec = doc->NewDeclaration( Value() );	// fixme: this will always allocate memory. Intern?
	 return dec;
}


bool XMLDeclaration::ShallowEqual( const XMLNode* compare ) const
{
	 TIXMLASSERT( compare );
	 const XMLDeclaration* declaration = compare->ToDeclaration();
	 return declaration && StringEqual( declaration->Value(), Value(), IsCaseInsensitive() );
}



bool XMLDeclaration::Accept( XMLVisitor* visitor ) const
{
	 TIXMLASSERT( visitor );
	 return visitor->Visit( *this );
}

// --------- XMLUnknown ---------- //

XMLUnknown::XMLUnknown( XMLDocument* doc ) : XMLNode( doc )
{
}


XMLUnknown::~XMLUnknown()
{
}


char* XMLUnknown::ParseDeep( char* p, StrPair*, uint* curLineNumPtr )
{
	 // Unknown parses as text.
	 p = _value.ParseText( p, ">", StrPair::NEEDS_NEWLINE_NORMALIZATION, *curLineNumPtr );
	 if ( !p ) {
		  _document->SetError( XML_ERROR_PARSING_UNKNOWN, _parseLineNum, 0 );
	 }
	 return p;
}


XMLNode* XMLUnknown::ShallowClone( XMLDocument* doc ) const
{
	 if ( !doc ) {
		  doc = _document;
	 }
	 XMLUnknown* text = doc->NewUnknown( Value() );	// fixme: this will always allocate memory. Intern?
	 return text;
}


bool XMLUnknown::ShallowEqual( const XMLNode* compare ) const
{
	 TIXMLASSERT( compare );
	 const XMLUnknown* unknown = compare->ToUnknown();
	 return unknown && StringEqual( unknown->Value(), Value(), IsCaseInsensitive() );
}


bool XMLUnknown::Accept( XMLVisitor* visitor ) const
{
	 TIXMLASSERT( visitor );
	 return visitor->Visit( *this );
}

// --------- XMLAttribute ---------- //

α XMLAttribute::Name()Ι->sv
{
	 return _name.GetStr();
}

α XMLAttribute::Value()Ι->sv
{
	 return _value.GetStr();
}

char* XMLAttribute::ParseDeep( char* p, XMLDocument& doc )
{
	if( p = _name.ParseName( p ); !p || !*p )
		return nullptr;

	p = SkipWhiteSpace( p, &doc.Line() );
	if ( *p != '=' )
	{
		if( char ch = *p; ch=='>' || ch=='/' )
		{
			SetName( _name.GetStr() );
			*p=ch;
		}
		else
			p = SkipWhiteSpace( p, &doc.Line() );
		return p;
	}

	++p;	// move up to opening quote
	p = SkipWhiteSpace( p, &doc.Line() );
	if ( *p != '\"' && *p != '\'' )
	{
		char* pStart = p; char ch = *p;
		for(  ; ch && !::isspace(ch) && ch!='/' && ch!='>'; ch = *++p );
		*p = 0;
		_value.SetStr( pStart );
		*p = ch;
	}
	else
	{
		std::array<char,2> rg{ *p++, 0 };
		p = _value.ParseText( p, rg.data(), doc.ProcessEntities() ? StrPair::ATTRIBUTE_VALUE : StrPair::ATTRIBUTE_VALUE_LEAVE_ENTITIES, doc.Line() );
		if( doc.Fix && isalpha(*p) )// fix(ignore) quotes inside quotes... style="font-family: "Times New Roman", Times, serif;"
		{
			BREAK;
			for( ++p ; p && *p && *p!=rg[0]; ++p );//to closing inner quote
			for( ++p ; p && *p && *p!=rg[0]; ++p );//to closing quote
			++p;
		}
	}
	return p;
}


void XMLAttribute::SetName( sv n )
{
	 _name.SetStr( n );
}

/*
XMLError XMLAttribute::QueryIntValue( int* value ) const
{
	 if ( ToInt( Value(), value )) {
		  return XML_SUCCESS;
	 }
	 return XML_WRONG_ATTRIBUTE_TYPE;
}


XMLError XMLAttribute::QueryUnsignedValue( unsigned int* value ) const
{
	 if ( ToUnsigned( Value(), value )) {
		  return XML_SUCCESS;
	 }
	 return XML_WRONG_ATTRIBUTE_TYPE;
}


XMLError XMLAttribute::QueryInt64Value(int64_t* value) const
{
	if (XMLUtil::ToInt64(Value(), value)) {
		return XML_SUCCESS;
	}
	return XML_WRONG_ATTRIBUTE_TYPE;
}


XMLError XMLAttribute::QueryUnsigned64Value(uint64_t* value) const
{
	 if(ToUnsigned64(Value(), value)) {
		  return XML_SUCCESS;
	 }
	 return XML_WRONG_ATTRIBUTE_TYPE;
}


XMLError XMLAttribute::QueryBoolValue( bool* value ) const
{
	 if ( ToBool( Value(), value )) {
		  return XML_SUCCESS;
	 }
	 return XML_WRONG_ATTRIBUTE_TYPE;
}


XMLError XMLAttribute::QueryFloatValue( float* value ) const
{
	 if ( ToFloat( Value(), value )) {
		  return XML_SUCCESS;
	 }
	 return XML_WRONG_ATTRIBUTE_TYPE;
}


XMLError XMLAttribute::QueryDoubleValue( double* value ) const
{
	 if ( ToDouble( Value(), value )) {
		  return XML_SUCCESS;
	 }
	 return XML_WRONG_ATTRIBUTE_TYPE;
}
*/

void XMLAttribute::SetAttribute( sv v )
{
	 _value.SetStr( v );
}


void XMLAttribute::SetAttribute( int v )
{
	 char buf[BUF_SIZE];
	 ToStr( v, buf, BUF_SIZE );
	 _value.SetStr( buf );
}


void XMLAttribute::SetAttribute( unsigned v )
{
	 char buf[BUF_SIZE];
	 ToStr( v, buf, BUF_SIZE );
	 _value.SetStr( buf );
}


void XMLAttribute::SetAttribute( bool v )
{
	 char buf[BUF_SIZE];
	 ToStr( v, buf, BUF_SIZE );
	 _value.SetStr( buf );
}

void XMLAttribute::SetAttribute( int64_t v ){ _value.SetStr(std::to_string(v)); }
void XMLAttribute::SetAttribute( uint64_t v ){ _value.SetStr(std::to_string(v)); }
void XMLAttribute::SetAttribute( double v ){ _value.SetStr(std::to_string(v)); }
void XMLAttribute::SetAttribute( float v ){ _value.SetStr(std::to_string(v)); }


// --------- XMLElement ---------- //
XMLElement::XMLElement( XMLDocument* doc ) : XMLNode( doc ),
	 _closingType( OPEN ),
	 _rootAttribute( 0 )
{
}


XMLElement::~XMLElement()
{
	 while( _rootAttribute ) {
		  XMLAttribute* next = _rootAttribute->_next;
		  DeleteAttribute( _rootAttribute );
		  _rootAttribute = next;
	 }
}

α XMLElement::operator[]( sv n )Ι->const XMLAttribute*
{
	auto p = _rootAttribute;
	for( ; p; p = p->_next )
	{
		if( p->Name()==n )
			break;
	}
	return p;
}

const XMLAttribute* XMLElement::FindAttribute( sv name ) const
{
	 for( XMLAttribute* a = _rootAttribute; a; a = a->_next )
	 {
		  if ( StringEqual( a->Name(), name, IsCaseInsensitive() ) )
				return a;
	 }
	 return nullptr;
}

/*
const char* XMLElement::Attribute( const char* name, const char* value ) const
{
	 const XMLAttribute* a = FindAttribute( name );
	 if ( !a ) {
		  return 0;
	 }
	 if ( !value || StringEqual( a->Value(), value )) {
		  return a->Value();
	 }
	 return 0;
}
* /

int XMLElement::IntAttribute(const char* name, int defaultValue) const
{
	int i = defaultValue;
	QueryIntAttribute(name, &i);
	return i;
}

unsigned XMLElement::UnsignedAttribute(const char* name, unsigned defaultValue) const
{
	unsigned i = defaultValue;
	QueryUnsignedAttribute(name, &i);
	return i;
}

int64_t XMLElement::Int64Attribute(const char* name, int64_t defaultValue) const
{
	int64_t i = defaultValue;
	QueryInt64Attribute(name, &i);
	return i;
}

uint64_t XMLElement::Unsigned64Attribute(const char* name, uint64_t defaultValue) const
{
	uint64_t i = defaultValue;
	QueryUnsigned64Attribute(name, &i);
	return i;
}

bool XMLElement::BoolAttribute(const char* name, bool defaultValue) const
{
	bool b = defaultValue;
	QueryBoolAttribute(name, &b);
	return b;
}

double XMLElement::DoubleAttribute(const char* name, double defaultValue) const
{
	double d = defaultValue;
	QueryDoubleAttribute(name, &d);
	return d;
}

float XMLElement::FloatAttribute(const char* name, float defaultValue) const
{
	float f = defaultValue;
	QueryFloatAttribute(name, &f);
	return f;
}
*/
sv XMLElement::Text()Ι
{
	let* node = FirstChild();
	for( ; node && node->ToComment(); node = node->NextSibling() );

	let txt = node ? node->ToText() : nullptr;
	return txt ? txt->Value() : sv{};
}

/*
α XMLElement::FirstText()Ι->string
{
	auto text = UnEscape( Text() );
	Trim( text );
	if( text.empty() )
		text = NextText();
	return text;
}
α XMLElement::NextText( / *const XMLElement* pAnchor* / )Ι->string
{
	string text;
	// if( const XMLNode* n{FirstChild()}; n  )
	// {
	// 	auto p2 = n->ToElement();
	// 	string x{ p2 ? Trim(p2->Text()) : "null" };
	// 	DBG( "[{}]='{}'", Trim(n->value()), x );
	// }
	if( auto c = pAnchor ? nullptr : FirstChild(); c )//pAnchor's siblings already searched.
	{
		text = c->FirstText();
	}
	if( auto s = NextSiblingElement(); s && text.empty() )
		text = s->FirstText();
	if( auto s = NextSibling(); s && text.empty() )
	{
		if( let p=s->ToText(); p )
		{
			text = UnEscape( p->value() );
			Trim( text );
		}
	}
	if( text.empty() && Parent() )
		text = ParentElement()->NextText( this );
	return text;
}*/

α XMLElement::ChildText( sv elementName )ε->sv
{
	auto p = FirstChildElement( elementName ); THROW_IF( !p, "Could not find {} in {}", elementName, Name() );
	return p->Text();
}
α XMLElement::TryChildText( sv elementName )ι->sv
{
	auto p = FirstChildElement( elementName );
	return p ? p->Text() : sv{};
}
α XMLElement::TryChildAttribute( sv elementName, sv attributeName )Ι->sv
{
	auto p = FirstChildElement( elementName );
	return p ? p->Attr( attributeName ) : sv{};
}

void XMLElement::SetText( sv inText )
{
	if( FirstChild() && FirstChild()->ToText() )
		FirstChild()->SetValue( inText );
	else
		InsertFirstChild( GetDocument().NewText(inText) );
}

void	XMLElement::SetText( const char* inText )
{
	if ( FirstChild() && FirstChild()->ToText() )
		FirstChild()->SetValue( inText );
	else {
		XMLText*	theText = GetDocument().NewText( inText );
		InsertFirstChild( theText );
	}
}


void XMLElement::SetText( int v )
{
	 char buf[BUF_SIZE];
	 ToStr( v, buf, BUF_SIZE );
	 SetText( buf );
}


void XMLElement::SetText( unsigned v )
{
	 char buf[BUF_SIZE];
	 ToStr( v, buf, BUF_SIZE );
	 SetText( buf );
}


void XMLElement::SetText( int64_t v ){	SetText( std::to_string(v) ); }
void XMLElement::SetText( uint64_t v ){ SetText( std::to_string(v) ); }
void XMLElement::SetText( float v ){ SetText( std::to_string(v) ); }
void XMLElement::SetText( double v ){ SetText( std::to_string(v) ); }

void XMLElement::SetText( bool v )
{
	 char buf[BUF_SIZE];
	 ToStr( v, buf, BUF_SIZE );
	 SetText( buf );
}



/*
XMLError XMLElement::QueryIntText( int* ival ) const
{
	 if ( FirstChild() && FirstChild()->ToText() ) {
		  let t = FirstChild()->Value();
		  if ( ToInt( t, ival ) ) {
				return XML_SUCCESS;
		  }
		  return XML_CAN_NOT_CONVERT_TEXT;
	 }
	 return XML_NO_TEXT_NODE;
}

XMLError XMLElement::QueryUnsignedText( unsigned* uval ) const
{
	 if ( FirstChild() && FirstChild()->ToText() ) {
		  auto t = FirstChild()->Value();
		  if ( ToUnsigned( t, uval ) ) {
				return XML_SUCCESS;
		  }
		  return XML_CAN_NOT_CONVERT_TEXT;
	 }
	 return XML_NO_TEXT_NODE;
}


XMLError XMLElement::QueryInt64Text(int64_t* ival) const
{
	if (FirstChild() && FirstChild()->ToText()) {
		auto t = FirstChild()->Value();
		if (ToInt64(t, ival)) {
			return XML_SUCCESS;
		}
		return XML_CAN_NOT_CONVERT_TEXT;
	}
	return XML_NO_TEXT_NODE;
}


XMLError XMLElement::QueryUnsigned64Text(uint64_t* ival) const
{
	 if(FirstChild() && FirstChild()->ToText()) {
		  let t = FirstChild()->Value();
		  if(ToUnsigned64(t, ival)) {
				return XML_SUCCESS;
		  }
		  return XML_CAN_NOT_CONVERT_TEXT;
	 }
	 return XML_NO_TEXT_NODE;
}


XMLError XMLElement::QueryBoolText( bool* bval ) const
{
	 if ( FirstChild() && FirstChild()->ToText() ) {
		  let t = FirstChild()->Value();
		  if ( ToBool( t, bval ) ) {
				return XML_SUCCESS;
		  }
		  return XML_CAN_NOT_CONVERT_TEXT;
	 }
	 return XML_NO_TEXT_NODE;
}


XMLError XMLElement::QueryDoubleText( double* dval ) const
{
	 if ( FirstChild() && FirstChild()->ToText() ) {
		  let t = FirstChild()->Value();
		  if ( ToDouble( t, dval ) ) {
				return XML_SUCCESS;
		  }
		  return XML_CAN_NOT_CONVERT_TEXT;
	 }
	 return XML_NO_TEXT_NODE;
}


XMLError XMLElement::QueryFloatText( float* fval ) const
{
	 if ( FirstChild() && FirstChild()->ToText() ) {
		  let t = FirstChild()->Value();
		  if ( ToFloat( t, fval ) ) {
				return XML_SUCCESS;
		  }
		  return XML_CAN_NOT_CONVERT_TEXT;
	 }
	 return XML_NO_TEXT_NODE;
}

int XMLElement::IntText(int defaultValue) const
{
	int i = defaultValue;
	QueryIntText(&i);
	return i;
}

unsigned XMLElement::UnsignedText(unsigned defaultValue) const
{
	unsigned i = defaultValue;
	QueryUnsignedText(&i);
	return i;
}

int64_t XMLElement::Int64Text(int64_t defaultValue) const
{
	int64_t i = defaultValue;
	QueryInt64Text(&i);
	return i;
}

uint64_t XMLElement::Unsigned64Text(uint64_t defaultValue) const
{
	uint64_t i = defaultValue;
	QueryUnsigned64Text(&i);
	return i;
}

bool XMLElement::BoolText(bool defaultValue) const
{
	bool b = defaultValue;
	QueryBoolText(&b);
	return b;
}

double XMLElement::DoubleText(double defaultValue) const
{
	double d = defaultValue;
	QueryDoubleText(&d);
	return d;
}

float XMLElement::FloatText(float defaultValue) const
{
	float f = defaultValue;
	QueryFloatText(&f);
	return f;
}
*/

XMLAttribute* XMLElement::FindOrCreateAttribute( sv name )
{
	 XMLAttribute* last = 0;
	 XMLAttribute* attrib = 0;
	 for( attrib = _rootAttribute;
				attrib;
				last = attrib, attrib = attrib->_next ) {
		  if ( StringEqual( attrib->Name(), name, IsCaseInsensitive() ) ) {
				break;
		  }
	 }
	 if ( !attrib ) {
		  attrib = CreateAttribute();
		  TIXMLASSERT( attrib );
		  if ( last ) {
				TIXMLASSERT( last->_next == 0 );
				last->_next = attrib;
		  }
		  else {
				TIXMLASSERT( _rootAttribute == 0 );
				_rootAttribute = attrib;
		  }
		  attrib->SetName( name );
	 }
	 return attrib;
}


void XMLElement::DeleteAttribute( sv name )
{
	 XMLAttribute* prev = 0;
	 for( XMLAttribute* a=_rootAttribute; a; a=a->_next ) {
		  if ( StringEqual( name, a->Name(), IsCaseInsensitive() ) ) {
				if ( prev ) {
					 prev->_next = a->_next;
				}
				else {
					 _rootAttribute = a->_next;
				}
				DeleteAttribute( a );
				break;
		  }
		  prev = a;
	 }
}


α XMLElement::ParseAttributes( char* p, uint& line )->char*
{
	for( XMLAttribute* prevAttribute{}; p && (p = SkipWhiteSpace(p, &line)); )
	{
		if( !*p )
		{
			BREAK;
			_document->SetError( XML_ERROR_PARSING_ELEMENT, _parseLineNum, "XMLElement name=%s", Name() );
			p = nullptr;
			break;
		}

		if( IsNameStartChar( (unsigned char) *p ) )
		{
			XMLAttribute* attrib = CreateAttribute();
			TIXMLASSERT( attrib );
			attrib->_parseLineNum = _document->_parseCurLineNum;

			let attrLineNum = attrib->_parseLineNum;
			//DEBUG_IF( line==89 );

			p = attrib->ParseDeep( p, *_document );

			let n = attrib->Name();
			if( !p ) //Attribute(n)
			{
				BREAK;
				DeleteAttribute( attrib );
				_document->SetError( XML_ERROR_PARSING_ATTRIBUTE, attrLineNum, "XMLElement name=%s", Name() );
				break;
			}
			if( (*this)[n] )
			{
				if( GetDocument().Fix )
					continue;
				BREAK;
				DeleteAttribute( attrib );
				_document->SetError( XML_ERROR_PARSING_ATTRIBUTE, attrLineNum, "XMLElement name=%s, duplicate attribute %s", Name(), n );
				break;
			}
			// There is a minor bug here: if the attribute in the source xml document is duplicated, it will not be detected and the attribute will be doubly added. However, tracking the 'prevAttribute' avoids re-scanning the attribute list. Preferring performance for now, may reconsider in the future.
			if ( prevAttribute )
			{
				TIXMLASSERT( prevAttribute->_next == 0 );
				prevAttribute->_next = attrib;
			}
			else
			{
				TIXMLASSERT( _rootAttribute == 0 );
				_rootAttribute = attrib;
			}
			prevAttribute = attrib;
		}
		else if ( *p == '>' )
		{
			++p;
			break;
		}
		else if ( *p == '/' && *(p+1) == '>' )
		{
			_closingType = CLOSED;
			p+=2;
			break;
			//return p+2;	// done; sealed element.
		}
		else
		{
			if( GetDocument().Fix && *p=='<' )
			{
				++p;
				continue;
			}

			BREAK;
			_document->SetError( XML_ERROR_PARSING_ELEMENT, _parseLineNum, 0 );
			p = nullptr;
		}
	}
	return p;
}

α XMLElement::DeleteAttribute( XMLAttribute* attribute )->void
{
	 if ( attribute == 0 ) {
		  return;
	 }
	 MemPool* pool = attribute->_memPool;
	 attribute->~XMLAttribute();
	 pool->Free( attribute );
}

XMLAttribute* XMLElement::CreateAttribute()
{
	 TIXMLASSERT( sizeof( XMLAttribute ) == _document->_attributePool.ItemSize() );
	 XMLAttribute* attrib = new (_document->_attributePool.Alloc() ) XMLAttribute();
	 TIXMLASSERT( attrib );
	 attrib->_memPool = &_document->_attributePool;
	 attrib->_memPool->SetTracked();
	 return attrib;
}


XMLElement* XMLElement::InsertNewChildElement(const char* name)
{
	 XMLElement* node = _document->NewElement(name);
	 return InsertEndChild(node) ? node : 0;
}

XMLComment* XMLElement::InsertNewComment(const char* comment)
{
	 XMLComment* node = _document->NewComment(comment);
	 return InsertEndChild(node) ? node : 0;
}

XMLText* XMLElement::InsertNewText(const char* text)
{
	 XMLText* node = _document->NewText(text);
	 return InsertEndChild(node) ? node : 0;
}

XMLDeclaration* XMLElement::InsertNewDeclaration(const char* text)
{
	 XMLDeclaration* node = _document->NewDeclaration(text);
	 return InsertEndChild(node) ? node : 0;
}

XMLUnknown* XMLElement::InsertNewUnknown(const char* text)
{
	 XMLUnknown* node = _document->NewUnknown(text);
	 return InsertEndChild(node) ? node : 0;
}



//
//	<ele></ele>
//	<ele>foo<b>bar</b></ele>
//
char* XMLElement::ParseDeep( char* p, StrPair* parentEndTag, uint* curLineNumPtr )
{
	 // Read the element name.
	 p = SkipWhiteSpace( p, curLineNumPtr );

	 // The closing element is the </element> form. It is
	 // parsed just like a regular element then deleted from
	 // the DOM.
	 if ( *p == '/' ) {
		  _closingType = CLOSING;
		  ++p;
	 }

	 p = _value.ParseName( p );
	 if ( _value.Empty() ) {
		  return 0;
	 }

	 p = ParseAttributes( p, *curLineNumPtr );
	 if ( !p || !*p || _closingType != OPEN ) {
		  return p;
	 }

	 p = XMLNode::ParseDeep( p, parentEndTag, curLineNumPtr );//~~
	 return p;
}



XMLNode* XMLElement::ShallowClone( XMLDocument* doc ) const
{
	 if ( !doc ) {
		  doc = _document;
	 }
	 XMLElement* element = doc->NewElement( Value() );					// fixme: this will always allocate memory. Intern?
	 for( const XMLAttribute* a=FirstAttribute(); a; a=a->Next() ) {
		  element->SetAttribute( a->Name(), a->Value() );					// fixme: this will always allocate memory. Intern?
	 }
	 return element;
}


bool XMLElement::ShallowEqual( const XMLNode* compare ) const
{
	 TIXMLASSERT( compare );
	 const XMLElement* other = compare->ToElement();
	 if ( other && StringEqual( other->Name(), Name(), IsCaseInsensitive() )) {

		  const XMLAttribute* a=FirstAttribute();
		  const XMLAttribute* b=other->FirstAttribute();

		  while ( a && b )
		  {
				if ( !StringEqual(a->Value(), b->Value(), IsCaseInsensitive()) )
					 return false;

				a = a->Next();
				b = b->Next();
		  }
		  if ( a || b )
				return false;

		  return true;
	 }
	 return false;
}


bool XMLElement::Accept( XMLVisitor* visitor ) const
{
	 TIXMLASSERT( visitor );
	 if ( visitor->VisitEnter( *this, _rootAttribute ) ) {
		  for ( const XMLNode* node=FirstChild(); node; node=node->NextSibling() ) {
				if ( !node->Accept( visitor ) ) {
					 break;
				}
		  }
	 }
	 return visitor->VisitExit( *this );
}


// --------- XMLDocument ----------- //

// Warning: List must match 'enum XMLError'
const char* XMLDocument::_errorNames[XML_ERROR_COUNT] = {
	 "XML_SUCCESS",
	 "XML_NO_ATTRIBUTE",
	 "XML_WRONG_ATTRIBUTE_TYPE",
	 "XML_ERROR_FILE_NOT_FOUND",
	 "XML_ERROR_FILE_COULD_NOT_BE_OPENED",
	 "XML_ERROR_FILE_READ_ERROR",
	 "XML_ERROR_PARSING_ELEMENT",
	 "XML_ERROR_PARSING_ATTRIBUTE",
	 "XML_ERROR_PARSING_TEXT",
	 "XML_ERROR_PARSING_CDATA",
	 "XML_ERROR_PARSING_COMMENT",
	 "XML_ERROR_PARSING_DECLARATION",
	 "XML_ERROR_PARSING_UNKNOWN",
	 "XML_ERROR_EMPTY_DOCUMENT",
	 "XML_ERROR_MISMATCHED_ELEMENT",
	 "XML_ERROR_PARSING",
	 "XML_CAN_NOT_CONVERT_TEXT",
	 "XML_NO_TEXT_NODE",
	"XML_ELEMENT_DEPTH_EXCEEDED"
};

XMLDocument::XMLDocument( std::string_view value, bool insensitive, bool fix, Jde::SL sl )ε:
	XMLDocument{ true, PRESERVE_WHITESPACE, insensitive, fix }
{
	if( Parse(value.data(), value.size()) ){
		if( let p = Settings::FindString( "/xml/errorFile" ); p ){
			std::ofstream os{ *p, std::ios::binary };
			os << value;
			//Logging::Log( Logging::MessageBase{ELogLevel::Error, ErrorStr(), p->c_str(), "XMLDocument::XMLDocument", (uint_least32_t)_errorLineNum}, _logTag );
			BREAK;
		}
		throw Jde::Exception{ sl, Jde::ELogLevel::Debug, "Could not parse '{}' - '{}'", value.substr(0, 100), ErrorStr() };
	}
	CHECK( RootElement() );
}
XMLDocument::XMLDocument( bool processEntities, Whitespace whitespaceMode, bool insensitive, bool fix ):
	XMLNode{ nullptr },
		Fix{ fix },
	 _writeBOM( false ),
	 _processEntities( processEntities ),
	 _errorID(XML_SUCCESS),
	 _whitespaceMode( whitespaceMode ),
	 _errorStr(),
	 _errorLineNum( 0 ),
	 _charBuffer( 0 ),
	_parseCurLineNum{ 0 },
	_parsingDepth(0),
	 _unlinked(),
	 _elementPool(),
	 _attributePool(),
	 _textPool(),
	 _commentPool(),
	 _isCaseInsensitive{insensitive}
{
	 // avoid VC++ C4355 warning about 'this' in initializer list (C4355 is off by default in VS2012+)
	 _document = this;
}


XMLDocument::~XMLDocument()
{
	 Clear();
}


void XMLDocument::MarkInUse(const XMLNode* const node)
{
	TIXMLASSERT(node);
	TIXMLASSERT(node->_parent == 0);

	for (int i = 0; i < _unlinked.Size(); ++i) {
		if (node == _unlinked[i]) {
			_unlinked.SwapRemove(i);
			break;
		}
	}
}

void XMLDocument::Clear()
{
	 DeleteChildren();
	while( _unlinked.Size()) {
		DeleteNode(_unlinked[0]);	// Will remove from _unlinked as part of delete.
	}

#ifdef TINYXML2_DEBUG
	 const bool hadError = Error();
#endif
	 ClearError();

	 delete [] _charBuffer;
	 _charBuffer = 0;
	_parsingDepth = 0;

#if 0
	 _textPool.Trace( "text" );
	 _elementPool.Trace( "element" );
	 _commentPool.Trace( "comment" );
	 _attributePool.Trace( "attribute" );
#endif

#ifdef TINYXML2_DEBUG
	 if ( !hadError ) {
		  TIXMLASSERT( _elementPool.CurrentAllocs()   == _elementPool.Untracked() );
		  TIXMLASSERT( _attributePool.CurrentAllocs() == _attributePool.Untracked() );
		  TIXMLASSERT( _textPool.CurrentAllocs()      == _textPool.Untracked() );
		  TIXMLASSERT( _commentPool.CurrentAllocs()   == _commentPool.Untracked() );
	 }
#endif
}


void XMLDocument::DeepCopy(XMLDocument* target) const
{
	TIXMLASSERT(target);
	 if (target == this) {
		  return; // technically success - a no-op.
	 }

	target->Clear();
	for (const XMLNode* node = this->FirstChild(); node; node = node->NextSibling()) {
		target->InsertEndChild(node->DeepClone(target));
	}
}

XMLElement* XMLDocument::NewElement( sv name )
{
	 XMLElement* ele = CreateUnlinkedNode<XMLElement>( _elementPool );
	 ele->SetName( name );
	 return ele;
}


XMLComment* XMLDocument::NewComment( sv str )
{
	 XMLComment* comment = CreateUnlinkedNode<XMLComment>( _commentPool );
	 comment->SetValue( str );
	 return comment;
}


XMLText* XMLDocument::NewText( sv str )
{
	 XMLText* text = CreateUnlinkedNode<XMLText>( _textPool );
	 text->SetValue( str );
	 return text;
}


XMLDeclaration* XMLDocument::NewDeclaration( sv str )
{
	 XMLDeclaration* dec = CreateUnlinkedNode<XMLDeclaration>( _commentPool );
	 dec->SetValue( str.size() ? str : "xml version=\"1.0\" encoding=\"UTF-8\"" );
	 return dec;
}


XMLUnknown* XMLDocument::NewUnknown( sv str )
{
	 XMLUnknown* unk = CreateUnlinkedNode<XMLUnknown>( _commentPool );
	 unk->SetValue( str );
	 return unk;
}
/*
static FILE* callfopen( const char* filepath, const char* mode )
{
	 TIXMLASSERT( filepath );
	 TIXMLASSERT( mode );
#if defined(_MSC_VER) && (_MSC_VER >= 1400 ) && (!defined WINCE)
	 FILE* fp = 0;
	 const errno_t err = fopen_s( &fp, filepath, mode );
	 if ( err ) {
		  return 0;
	 }
#else
	 FILE* fp = fopen( filepath, mode );
#endif
	 return fp;
}
*/
void XMLDocument::DeleteNode( XMLNode* node )	{
	 TIXMLASSERT( node );
	 TIXMLASSERT(node->_document == this );
	 if (node->_parent) {
		  node->_parent->DeleteChild( node );
	 }
	 else {
		  // Isn't in the tree.
		  // Use the parent delete.
		  // Also, we need to mark it tracked: we 'know'
		  // it was never used.
		  node->_memPool->SetTracked();
		  // Call the static XMLNode version:
		  XMLNode::DeleteNode(node);
	 }
}

/*
XMLError XMLDocument::LoadFile( const char* filename )
{
	 if ( !filename ) {
		  TIXMLASSERT( false );
		  SetError( XML_ERROR_FILE_COULD_NOT_BE_OPENED, 0, "filename=<null>" );
		  return _errorID;
	 }

	 Clear();
	 FILE* fp = callfopen( filename, "rb" );
	 if ( !fp ) {
		  SetError( XML_ERROR_FILE_NOT_FOUND, 0, "filename=%s", filename );
		  return _errorID;
	 }
	 LoadFile( fp );
	 fclose( fp );
	 return _errorID;
}

XMLError XMLDocument::LoadFile( FILE* fp )
{
	 Clear();

	 TIXML_FSEEK( fp, 0, SEEK_SET );
	 if ( fgetc( fp ) == EOF && ferror( fp ) != 0 ) {
		  SetError( XML_ERROR_FILE_READ_ERROR, 0, 0 );
		  return _errorID;
	 }

	 TIXML_FSEEK( fp, 0, SEEK_END );

	 unsigned long long filelength;
	 {
		  const long long fileLengthSigned = TIXML_FTELL( fp );
		  TIXML_FSEEK( fp, 0, SEEK_SET );
		  if ( fileLengthSigned == -1L ) {
				SetError( XML_ERROR_FILE_READ_ERROR, 0, 0 );
				return _errorID;
		  }
		  TIXMLASSERT( fileLengthSigned >= 0 );
		  filelength = static_cast<unsigned long long>(fileLengthSigned);
	 }

	 const size_t maxSizeT = static_cast<size_t>(-1);
	 // We'll do the comparison as an unsigned long long, because that's guaranteed to be at
	 // least 8 bytes, even on a 32-bit platform.
	 if ( filelength >= static_cast<unsigned long long>(maxSizeT) ) {
		  // Cannot handle files which won't fit in buffer together with null terminator
		  SetError( XML_ERROR_FILE_READ_ERROR, 0, 0 );
		  return _errorID;
	 }

	 if ( filelength == 0 ) {
		  SetError( XML_ERROR_EMPTY_DOCUMENT, 0, 0 );
		  return _errorID;
	 }

	 const size_t size = static_cast<size_t>(filelength);
	 TIXMLASSERT( _charBuffer == 0 );
	 _charBuffer = new char[size+1];
	 const size_t read = fread( _charBuffer, 1, size, fp );
	 if ( read != size ) {
		  SetError( XML_ERROR_FILE_READ_ERROR, 0, 0 );
		  return _errorID;
	 }

	 _charBuffer[size] = 0;

	 Parse();
	 return _errorID;
}


XMLError XMLDocument::SaveFile( const char* filename, bool compact )
{
	 if ( !filename ) {
		  TIXMLASSERT( false );
		  SetError( XML_ERROR_FILE_COULD_NOT_BE_OPENED, 0, "filename=<null>" );
		  return _errorID;
	 }

	 FILE* fp = callfopen( filename, "w" );
	 if ( !fp ) {
		  SetError( XML_ERROR_FILE_COULD_NOT_BE_OPENED, 0, "filename=%s", filename );
		  return _errorID;
	 }
	 SaveFile(fp, compact);
	 fclose( fp );
	 return _errorID;
}


XMLError XMLDocument::SaveFile( FILE* fp, bool compact )
{
	 // Clear any error from the last save, otherwise it will get reported
	 // for *this* call.
	 ClearError();
	 XMLPrinter stream( fp, compact );
	 Print( &stream );
	 return _errorID;
}
*/

XMLError XMLDocument::Parse( const char* p, size_t len )
{
	 Clear();

	 if ( len == 0 || !p || !*p ) {
		  SetError( XML_ERROR_EMPTY_DOCUMENT, 0, 0 );
		  return _errorID;
	 }
	 if ( len == static_cast<size_t>(-1) ) {
		  len = strlen( p );
	 }
	 TIXMLASSERT( _charBuffer == 0 );
	 _charBuffer = new char[ len+1 ];
	 memcpy( _charBuffer, p, len );
	 _charBuffer[len] = 0;

	 Parse();
	 if ( Error() ) {
		  // clean up now essentially dangling memory.
		  // and the parse fail can put objects in the
		  // pools that are dead and inaccessible.
		  DeleteChildren();
		  _elementPool.Clear();
		  _attributePool.Clear();
		  _textPool.Clear();
		  _commentPool.Clear();
	 }
	 return _errorID;
}

/*
void XMLDocument::Print( XMLPrinter* streamer ) const
{
	 if ( streamer ) {
		  Accept( streamer );
	 }
	 else {
		  XMLPrinter stdoutStreamer( stdout );
		  Accept( &stdoutStreamer );
	 }
}
*/

void XMLDocument::ClearError() {
	 _errorID = XML_SUCCESS;
	 _errorLineNum = 0;
	 _errorStr.Reset();
}


void XMLDocument::SetError( XMLError error, uint lineNum, const char* format, ... )
{
	 TIXMLASSERT( error >= 0 && error < XML_ERROR_COUNT );
	 _errorID = error;
	 _errorLineNum = lineNum;
	_errorStr.Reset();

	 const size_t BUFFER_SIZE = 1000;
	 char* buffer = new char[BUFFER_SIZE];

	 TIXMLASSERT(sizeof(error) <= sizeof(int));
	 TIXML_SNPRINTF(buffer, BUFFER_SIZE, "Error=%s ErrorID=%d (0x%x) Line number=%lu", ErrorIDToName(error), int(error), int(error), lineNum);

	if (format) {
		size_t len = strlen(buffer);
		TIXML_SNPRINTF(buffer + len, BUFFER_SIZE - len, ": ");
		len = strlen(buffer);

		va_list va;
		va_start(va, format);
		TIXML_VSNPRINTF(buffer + len, BUFFER_SIZE - len, format, va);
		va_end(va);
	}
	_errorStr.SetStr(buffer);
	delete[] buffer;
}


/*static*/ const char* XMLDocument::ErrorIDToName(XMLError errorID)
{
	TIXMLASSERT( errorID >= 0 && errorID < XML_ERROR_COUNT );
	 const char* errorName = _errorNames[errorID];
	 TIXMLASSERT( errorName && errorName[0] );
	 return errorName;
}

sv XMLDocument::ErrorStr() const
{
	return _errorStr.GetStr();
}

/*
void XMLDocument::PrintError() const
{
	 //printf( "%s\n", ErrorStr() );
}*/

const char* XMLDocument::ErrorName() const
{
	 return ErrorIDToName(_errorID);
}

void XMLDocument::Parse()
{
	 TIXMLASSERT( NoChildren() ); // Clear() must have been called previously
	 TIXMLASSERT( _charBuffer );
	 _parseCurLineNum = 1;
	 _parseLineNum = 1;
	 char* p = _charBuffer;
	 p = SkipWhiteSpace( p, &_parseCurLineNum );
	 p = const_cast<char*>( ReadBOM( p, &_writeBOM ) );
	 if ( !*p ) {
		  SetError( XML_ERROR_EMPTY_DOCUMENT, 0, 0 );
		  return;
	 }
	 ParseDeep(p, 0, &_parseCurLineNum );
}

void XMLDocument::PushDepth()
{
	_parsingDepth++;
	if (_parsingDepth == TINYXML2_MAX_ELEMENT_DEPTH) {
		SetError(XML_ELEMENT_DEPTH_EXCEEDED, _parseCurLineNum, "Element nesting is too deep." );
	}
}

void XMLDocument::PopDepth()
{
	TIXMLASSERT(_parsingDepth > 0);
	--_parsingDepth;
}

α XMLNode::IsCaseInsensitive()Ι->bool{ return GetDocument().IsCaseInsensitive(); }
α XMLNode::Find( const std::span<sv>& entries )Ι->const XMLElement*
{
	ASSERT( entries.size() );
	let p = FirstChildElement( entries[0] );
	return p && entries.size()>1 ? p->Find( {&entries[1], entries.size()-1} ) : p;
}

α XMLDocument::Find( const std::span<sv>& entries )Ι->const XMLElement*
{

	const XMLElement* y = nullptr;
	if( auto pRoot = RootElement(); pRoot && entries.size() && pRoot->Name()==entries[0] )
		y = entries.size()==1 ? pRoot : pRoot->Find( {&entries[1], entries.size()-1} );
	return y;
}

α XMLNode::NextHtmlText( iv prev )Ι->const XMLNode*
{
	const String text{ prev.size() ? prev : get<0>(HtmlText<String>()) };
	auto n = NextHtml();
	auto next{ n ? get<0>(n->HtmlText<String>()) : String{} };
	for( ; n->ToText() && (next.empty() || next==text); next = get<0>(n->HtmlText<String>()) )//text should have been included in parent.
		n = n->NextHtml();

	let equal = next.empty() || next==text;
	return !equal ? n : n ? n->NextHtmlText( text ) : nullptr;
}

α XMLNode::NextHtml( bool children, bool continuation )Ι->const XMLNode*
{
	let* y{ children && !IsHtmlStyle() ? FirstChild() : nullptr };

	if( !y && NextSibling() )
		y = NextSibling()->NextHtml();
	else if( y && y->IsHtmlStyle() && continuation )
		y = y->NextHtml( false );
	else if( y && y->ToText() && y->NextSibling() && y->NextSibling() && y->NextSibling()->IsHtmlStyle() ) //A<small>ggregate
		y = y->NextSibling()->NextHtml( false );
	else if( y && y->IsHtmlStyle() && continuation )
	{
		if( let s=y->NextSibling(); s )
			y = s->IsHtmlStyle() ? s->NextHtml() : s;
		else if( y->ToText() )
			y = y->NextHtml();
		else if( continuation )
			y = nullptr;
	}
	//else if( let* s{y && y->IsHtmlStyle() ? NextSibling() : nullptr}; s )
	//	y = s->IsHtmlStyle() ? s->NextHtml() : s;
	if( !y && Parent() )
		y = Parent()->NextHtml( false );
	return y;
}
α XMLNode::Next( bool children )Ι->const XMLNode*
{
	const XMLNode* y{ children ? FirstChild() : nullptr };
	if( const XMLNode* s{y ? nullptr : NextSibling()}; s )
		y = s;
	else if( const XMLNode* p{y ? nullptr : Parent()}; p )
		y = p->Next( false );
	return y;
}
/*
α FindTextCompare( sv x, const std::span<sv>& entries, bool stem )ι->bool
{
	let txt = UnEscape( x );
	auto f = []( auto x ){ return vector<CIString>{x.begin(), x.end()}; };
	let v = stem
		? f( StemmedWords(txt) )
		: f( Words(txt) );
	bool equal = entries.empty() && v.size();//empty entries==any text.
	if( !equal )
	{
		for( auto p=v.begin(); !equal && (p=std::find(p, v.end(), entries.front()))!=v.end(); ++p )
		{
			size_t i=std::distance( v.begin(), p );
			equal = v.size()-i>=entries.size();
			for( uint j=1; equal && j<entries.size(); ++j )
				equal = ToUpper( v[++i] ) == ToUpper( entries[j] );
		}
	}
	return equal;
}
*/
α XMLNode::FindOneOf( const vector<iv>& entries, bool stem, const XMLNode* pCalledFrom, bool searchChildren, optional<FindOneOfStruct>& entryLocation, vector<string> tags )Ι->const XMLNode*
{
	const XMLNode* pThis = this;
	if( tags.empty() )
	{
		for( const XMLNode* p=this; (p=p->Parent())!=nullptr && p->Value().size(); )
			tags.insert( tags.begin(), string{p->Value()} );
	}
start:
	let pElement = pThis->ToElement();
	static vector<tuple<uint,String>> tags2;
	if( pElement )
	{
		//DEBUG_IF( pThis->Value()=="body" );
		Debug( _tags, "[{}]{}", pThis->GetLineNum(), pThis->Value() );
		tags2.push_back( make_tuple(pThis->GetLineNum(), String(pThis->Value<iv>())) );
		tags.push_back( string{pThis->Value()} );
	}
	const XMLNode* y{};
	//DEBUG_IF( pThis->_parseLineNum==138/*&& entries.front()!="NAME"*/ );

	string where;
	if( let c{searchChildren ? pThis->FirstChild() : nullptr}; c )
	{
		//auto foo = c->Value<iv>();
		//DEBUG_IF( foo=="Sayuri Childs, Chief Compliance Officer" );
		y = c->FindOneOf( entries, stem, pThis, true, entryLocation, tags );
	}
	if( auto p{y || pThis->ToElement() ? nullptr : pThis}; p )//element child above would have picked up.
	{
		let textNode = p->HtmlText<String>();
		pThis = get<1>( textNode );
		if( let text = get<0>(textNode); text.size() )
		{
			if( entryLocation )
			{
				bool equal = false; uint i = entryLocation->NextEntry, size;
				auto f = [&]( auto words )
				{
					size = words.size();
					for( uint j=0; i<entries.size() && j<size && (equal = entries[i]==words[j]); ++i, ++j );
				};
				if( stem )
					f( Str::StemmedWords<iv>(text) );
				else
					f( Str::Words<iv>(text) );
				if( equal && i==entries.size() )
					y = entryLocation->StartNodePtr;
				else if( equal )//all matched, just not enough
				{
				//	BREAK;//step through to make sure logic correct.
					entryLocation->NextEntry=i;
				}
				else
					entryLocation = nullopt;
			}
			if( !y && !entryLocation )
			{
				optional<Str::FindPhraseResult> result;
				if( result = Str::FindPhrase(text, entries, stem); result && result->NextEntry>=entries.size() )
					y = p;
				else if( result )
				{
					ASSERT( p->ToText() );
					entryLocation = FindOneOfStruct{ result.value(), p->Parent() };
				}
			}
		}
	}
	if( pElement )
	{
		tags.pop_back();
		tags2.pop_back();
		Debug( _tags, "~[{}]{}", pThis->GetLineNum(), pThis->Value() );
	}
	if( const XMLNode* n{pThis->NextSibling()}; !y && n )//<divs>
	{
		pThis = n; searchChildren = true;
		//entryLocation = {}; don't zero out, could be half way through
		goto start;//stack-overflow
	}
	else if( const XMLNode* p{ !y && pThis->Parent() && pThis->Parent()!=pCalledFrom ? pThis->Parent() : nullptr}; p )//td
		y = p->FindOneOf( entries, stem, pCalledFrom, false, entryLocation, tags.size() ? vector<string>{tags.begin(), tags.begin()+tags.size()-1} : vector<string>{} );

	return y;
}

α XMLNode::Parent( sv elementName )Ι->const XMLElement*
{
	const XMLElement* y = nullptr;
	for( const XMLNode* p=Parent(); !y && p; p=p->Parent() )
		y = p->ToElementWithName( elementName );

	return y;
}
/*
XMLPrinter::XMLPrinter( FILE* file, bool compact, int depth ) :
	 _elementJustOpened( false ),
	 _stack(),
	 _firstElement( true ),
	 _fp( file ),
	 _depth( depth ),
	 _textDepth( -1 ),
	 _processEntities( true ),
	 _compactMode( compact ),
	 _buffer()
{
	 for( int i=0; i<ENTITY_RANGE; ++i ) {
		  _entityFlag[i] = false;
		  _restrictedEntityFlag[i] = false;
	 }
	 for( let& entity : _entities )
	 {
		  const char entityValue = entity.value;
		  const unsigned char flagIndex = static_cast<unsigned char>(entityValue);
		  TIXMLASSERT( flagIndex < ENTITY_RANGE );
		  _entityFlag[flagIndex] = true;
	 }
	 _restrictedEntityFlag[static_cast<unsigned char>('&')] = true;
	 _restrictedEntityFlag[static_cast<unsigned char>('<')] = true;
	 _restrictedEntityFlag[static_cast<unsigned char>('>')] = true;	// not required, but consistency is nice
	 _buffer.Push( 0 );
}


void XMLPrinter::Print( const char* format, ... )
{
	 va_list     va;
	 va_start( va, format );

	 if ( _fp ) {
		  vfprintf( _fp, format, va );
	 }
	 else {
		  const int len = TIXML_VSCPRINTF( format, va );
		  // Close out and re-start the va-args
		  va_end( va );
		  TIXMLASSERT( len >= 0 );
		  va_start( va, format );
		  TIXMLASSERT( _buffer.Size() > 0 && _buffer[_buffer.Size() - 1] == 0 );
		  char* p = _buffer.PushArr( len ) - 1;	// back up over the null terminator.
		TIXML_VSNPRINTF( p, len+1, format, va );
	 }
	 va_end( va );
}


void XMLPrinter::Write( const char* data, size_t size )
{
	 if ( _fp ) {
		  fwrite ( data , sizeof(char), size, _fp);
	 }
	 else {
		  char* p = _buffer.PushArr( static_cast<int>(size) ) - 1;   // back up over the null terminator.
		  memcpy( p, data, size );
		  p[size] = 0;
	 }
}


void XMLPrinter::Putc( char ch )
{
	 if ( _fp ) {
		  fputc ( ch, _fp);
	 }
	 else {
		  char* p = _buffer.PushArr( sizeof(char) ) - 1;   // back up over the null terminator.
		  p[0] = ch;
		  p[1] = 0;
	 }
}


void XMLPrinter::PrintSpace( int depth )
{
	 for( int i=0; i<depth; ++i ) {
		  Write( "    " );
	 }
}


void XMLPrinter::PrintString( const char* p, bool restricted )
{
	 // Look for runs of bytes between entities to print.
	 const char* q = p;

	 if ( _processEntities ) {
		  const bool* flag = restricted ? _restrictedEntityFlag : _entityFlag;
		  while ( *q ) {
				TIXMLASSERT( p <= q );
				// Remember, char is sometimes signed. (How many times has that bitten me?)
				if ( *q > 0 && *q < ENTITY_RANGE ) {
					 // Check for entities. If one is found, flush
					 // the stream up until the entity, write the
					 // entity, and keep looking.
					 if ( flag[static_cast<unsigned char>(*q)] ) {
						  while ( p < q ) {
								const size_t delta = q - p;
								const int toPrint = ( INT_MAX < delta ) ? INT_MAX : static_cast<int>(delta);
								Write( p, toPrint );
								p += toPrint;
						  }
						  bool entityPatternPrinted = false;
						  for( let& entity : _entities )
						  {
								if( entity.value != *q )
									continue;
								Putc( '&' );
								Write( entity.Pattern.data(), entity.Pattern.size() );
								Putc( ';' );
								entityPatternPrinted = true;
								break;
						  }
						  if ( !entityPatternPrinted ) {
								// TIXMLASSERT( entityPatternPrinted ) causes gcc -Wunused-but-set-variable in release
								TIXMLASSERT( false );
						  }
						  ++p;
					 }
				}
				++q;
				TIXMLASSERT( p <= q );
		  }
		  // Flush the remaining string. This will be the entire
		  // string if an entity wasn't found.
		  if ( p < q ) {
				const size_t delta = q - p;
				const int toPrint = ( INT_MAX < delta ) ? INT_MAX : static_cast<int>(delta);
				Write( p, toPrint );
		  }
	 }
	 else {
		  Write( p );
	 }
}


void XMLPrinter::PushHeader( bool writeBOM, bool writeDec )
{
	 if ( writeBOM ) {
		  static const unsigned char bom[] = { TIXML_UTF_LEAD_0, TIXML_UTF_LEAD_1, TIXML_UTF_LEAD_2, 0 };
		  Write( reinterpret_cast< const char* >( bom ) );
	 }
	 if ( writeDec ) {
		  PushDeclaration( "xml version=\"1.0\"" );
	 }
}

void XMLPrinter::PrepareForNewNode( bool compactMode )
{
	 SealElementIfJustOpened();

	 if ( compactMode ) {
		  return;
	 }

	 if ( _firstElement ) {
		  PrintSpace (_depth);
	 } else if ( _textDepth < 0) {
		  Putc( '\n' );
		  PrintSpace( _depth );
	 }

	 _firstElement = false;
}

void XMLPrinter::OpenElement( const char* name, bool compactMode )
{
	 PrepareForNewNode( compactMode );
	 _stack.Push( name );

	 Write ( "<" );
	 Write ( name );

	 _elementJustOpened = true;
	 ++_depth;
}


void XMLPrinter::PushAttribute( const char* name, const char* value )
{
	 TIXMLASSERT( _elementJustOpened );
	 Putc ( ' ' );
	 Write( name );
	 Write( "=\"" );
	 PrintString( value, false );
	 Putc ( '\"' );
}


void XMLPrinter::PushAttribute( const char* name, int v )
{
	 char buf[BUF_SIZE];
	 ToStr( v, buf, BUF_SIZE );
	 PushAttribute( name, buf );
}


void XMLPrinter::PushAttribute( const char* name, unsigned v )
{
	 char buf[BUF_SIZE];
	 ToStr( v, buf, BUF_SIZE );
	 PushAttribute( name, buf );
}


void XMLPrinter::PushAttribute(const char* name, int64_t v)
{
	char buf[BUF_SIZE];
	ToStr(v, buf, BUF_SIZE);
	PushAttribute(name, buf);
}


void XMLPrinter::PushAttribute(const char* name, uint64_t v)
{
	char buf[BUF_SIZE];
	ToStr(v, buf, BUF_SIZE);
	PushAttribute(name, buf);
}


void XMLPrinter::PushAttribute( const char* name, bool v )
{
	 char buf[BUF_SIZE];
	 ToStr( v, buf, BUF_SIZE );
	 PushAttribute( name, buf );
}


void XMLPrinter::PushAttribute( const char* name, double v )
{
	 char buf[BUF_SIZE];
	 ToStr( v, buf, BUF_SIZE );
	 PushAttribute( name, buf );
}


void XMLPrinter::CloseElement( bool compactMode )
{
	 --_depth;
	 const char* name = _stack.Pop();

	 if ( _elementJustOpened ) {
		  Write( "/>" );
	 }
	 else {
		  if ( _textDepth < 0 && !compactMode) {
				Putc( '\n' );
				PrintSpace( _depth );
		  }
		  Write ( "</" );
		  Write ( name );
		  Write ( ">" );
	 }

	 if ( _textDepth == _depth ) {
		  _textDepth = -1;
	 }
	 if ( _depth == 0 && !compactMode) {
		  Putc( '\n' );
	 }
	 _elementJustOpened = false;
}


void XMLPrinter::SealElementIfJustOpened()
{
	 if ( !_elementJustOpened ) {
		  return;
	 }
	 _elementJustOpened = false;
	 Putc( '>' );
}

void XMLPrinter::PushText( const char* text, bool cdata )
{
	 _textDepth = _depth-1;

	 SealElementIfJustOpened();
	 if ( cdata ) {
		  Write( "<![CDATA[" );
		  Write( text );
		  Write( "]]>" );
	 }
	 else {
		  PrintString( text, true );
	 }
}

void XMLPrinter::PushText( int64_t value )
{
	 char buf[BUF_SIZE];
	 ToStr( value, buf, BUF_SIZE );
	 PushText( buf, false );
}

void XMLPrinter::PushText( uint64_t value )
{
	char buf[BUF_SIZE];
	ToStr(value, buf, BUF_SIZE);
	PushText(buf, false);
}

void XMLPrinter::PushText( int value )
{
	 char buf[BUF_SIZE];
	 ToStr( value, buf, BUF_SIZE );
	 PushText( buf, false );
}

void XMLPrinter::PushText( unsigned value )
{
	 char buf[BUF_SIZE];
	 ToStr( value, buf, BUF_SIZE );
	 PushText( buf, false );
}

void XMLPrinter::PushText( bool value )
{
	 char buf[BUF_SIZE];
	 ToStr( value, buf, BUF_SIZE );
	 PushText( buf, false );
}

void XMLPrinter::PushText( float value )
{
	 char buf[BUF_SIZE];
	 ToStr( value, buf, BUF_SIZE );
	 PushText( buf, false );
}

void XMLPrinter::PushText( double value )
{
	 char buf[BUF_SIZE];
	 ToStr( value, buf, BUF_SIZE );
	 PushText( buf, false );
}

void XMLPrinter::PushComment( const char* comment )
{
	 PrepareForNewNode( _compactMode );

	 Write( "<!--" );
	 Write( comment );
	 Write( "-->" );
}

void XMLPrinter::PushDeclaration( const char* value )
{
	 PrepareForNewNode( _compactMode );

	 Write( "<?" );
	 Write( value );
	 Write( "?>" );
}

void XMLPrinter::PushUnknown( const char* value )
{
	 PrepareForNewNode( _compactMode );

	 Write( "<!" );
	 Write( value );
	 Putc( '>' );
}

bool XMLPrinter::VisitEnter( const XMLDocument& doc )
{
	 _processEntities = doc.ProcessEntities();
	 if ( doc.HasBOM() ) {
		  PushHeader( true, false );
	 }
	 return true;
}

bool XMLPrinter::VisitEnter( const XMLElement& element, const XMLAttribute* attribute )
{
	 const XMLElement* parentElem = 0;
	 if ( element.Parent() ) {
		  parentElem = element.Parent()->ToElement();
	 }
	 const bool compactMode = parentElem ? CompactMode( *parentElem ) : _compactMode;
	 OpenElement( element.Name(), compactMode );
	 while ( attribute ) {
		  PushAttribute( attribute->Name(), attribute->Value() );
		  attribute = attribute->Next();
	 }
	 return true;
}

bool XMLPrinter::VisitExit( const XMLElement& element )
{
	 CloseElement( CompactMode(element) );
	 return true;
}

bool XMLPrinter::Visit( const XMLText& text )
{
	 PushText( text.Value(), text.CData() );
	 return true;
}

bool XMLPrinter::Visit( const XMLComment& comment )
{
	 PushComment( comment.Value() );
	 return true;
}

bool XMLPrinter::Visit( const XMLDeclaration& declaration )
{
	 PushDeclaration( declaration.Value() );
	 return true;
}

bool XMLPrinter::Visit( const XMLUnknown& unknown )
{
	 PushUnknown( unknown.Value() );
	 return true;
}
*/
}
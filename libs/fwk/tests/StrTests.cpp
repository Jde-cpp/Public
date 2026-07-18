#include <jde/fwk/str.h>

#define let const auto

namespace Jde::Tests{
	using Bytes = vector<unsigned char>;

	//LTrim/RTrim fed sign-extended chars to iswspace (UB) and the multibyte space path was dead on Linux.
	TEST( StrTests, TrimAscii ){
		EXPECT_EQ( Str::Trim(sv{"  \t abc \n "}), "abc" );
		EXPECT_EQ( Str::LTrim(sv{"\t x"}), "x" );
		EXPECT_EQ( Str::RTrim(sv{"x \t"}), "x" );
		EXPECT_EQ( Str::Trim(string{"  abc  "}), "abc" );//string&& overload.
		EXPECT_EQ( Str::Trim(sv{"abc"}), "abc" );//no-op.
	}
	TEST( StrTests, TrimUnicodeSpace ){
		EXPECT_EQ( Str::LTrim(sv{"\xe2\x80\x89" "abc"}), "abc" );//U+2009 thin space - previously dead/UB on Linux.
		EXPECT_EQ( Str::LTrim(sv{"\xe2\x80\xaf" "abc"}), "abc" );//U+202F narrow no-break space.
		EXPECT_EQ( Str::LTrim(sv{"\xe2\x82\xac" "x"}), "\xe2\x82\xac" "x" );//U+20AC euro (not a space) preserved intact.
		EXPECT_EQ( Str::LTrim(sv{"\xe9" "x"}), "\xe9" "x" );//lone 0xE9 (invalid UTF-8 lead) - left as-is, no UB.
	}

	//Decode64 used to strip every trailing 0x00 byte (confusing them with padding), corrupting binary payloads such as RSA signatures ending in zero.
	TEST( StrTests, Decode64TrailingZeroBytes ){
		let bytes = Bytes{ 0xAB, 0x00 };
		ASSERT_EQ( Str::Encode64(bytes), "qwA=" );
		ASSERT_EQ( Str::Decode64<Bytes>("qwA="), bytes );
		ASSERT_EQ( Str::Decode64<Bytes>("qwA"), bytes );//jwt segments are unpadded.

		ASSERT_EQ( Str::Decode64<Bytes>("AAAA"), (Bytes{0x00, 0x00, 0x00}) );//all-zero payload previously decoded to empty.
	}

	//To is noexcept; the double specialization used to let stod's invalid_argument escape → std::terminate.
	TEST( StrTests, ToDoubleBadInput ){
		EXPECT_EQ( To<double>("abc"), 0.0 );
		EXPECT_EQ( To<double>(""), 0.0 );
		EXPECT_EQ( To<double>("1.5"), 1.5 );
	}

	TEST( StrTests, Decode64RoundTrip ){
		Bytes signature( 256 );
		for( uint i=0; i<signature.size(); ++i )
			signature[i] = (unsigned char)(i*7);
		signature.back() = 0x00;
		ASSERT_EQ( Str::Decode64<Bytes>(Str::Encode64(signature)), signature );
		ASSERT_EQ( Str::Decode64<Bytes>(Str::Encode64(signature, true), true), signature );

		let text = string{ "any carnal pleasure." };//lengths 20,19,18 exercise 0,1,2 padding chars.
		for( uint size : {20u, 19u, 18u} ){
			let payload = text.substr( 0, size );
			ASSERT_EQ( Str::Decode64(Str::Encode64(payload)), payload );
		}
	}
}

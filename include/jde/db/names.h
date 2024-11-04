#pragma once
#include <jde/framework/str.h>

#define BSV Str::bsv<typename T::traits_type>
#define RESULT std::basic_string<char,typename T::traits_type>
#define YRESULT std::basic_string<char,typename Y::traits_type>

namespace Jde::DB::Names{
	Ξ Capitalize( str name )ι->string{ ASSERT(name.size()>1); return string{(char)std::toupper(name[0])} + name.substr(1); }
	template<class X=string,class Y=string> Ω FromJson( Str::bsv<typename X::traits_type> jsonName )ι->YRESULT;
	template<class X=string,class Y=string> Ω ToJson( Str::bsv<typename X::traits_type> schemaName )ι->YRESULT;
	α ToSingular( sv plural )ι->string;
	template<class T=string> Ω ToPlural( BSV singular )ι->RESULT;
}
namespace Jde::DB{
#define let const auto
	template<class X,class Y> α Names::FromJson( Str::bsv<typename X::traits_type> jsonName )ι->YRESULT{
		YRESULT sqlName;
		for( uint i=0; i<jsonName.size(); ++i ){
			let ch = jsonName[i];
			if( std::isupper(ch) ){
				if( i!=0 && !std::isupper(jsonName[i-1]) )
					sqlName+="_";
				sqlName +=(char)std::tolower( ch );
			}
			else
				sqlName+=ch;
		}
		return sqlName;
	}
	template<class X,class Y> α Names::ToJson( Str::bsv<typename X::traits_type> schemaName )ι->YRESULT{
		std::ostringstream j;
		bool upper = false;
		for( let ch : schemaName ){
			if( ch=='_' )
				upper = true;
			else if( upper ){
				j << (char)std::toupper( ch );
				upper = false;
			}
			else if( j.tellp()==0 )
				j << (char)tolower( ch );
			else
				j << ch;
		}
		return j.str();
	}
	Ξ Names::ToSingular( sv plural )ι->string{
		string y{ plural };
		if( plural.ends_with("ies") )
			y = string{plural.substr( 0, plural.size()-3 )}+"y";
		else if( plural.ends_with('s') )
			y = plural.substr( 0, plural.size()-1 );
		return y;
	}
	Ŧ Names::ToPlural( BSV singular )ι->RESULT{
		RESULT y{ singular };
		if( singular=="acl" )
			y = singular;
		else if( singular.ends_with("y") )
			y = RESULT{ singular }.substr(0, singular.size()-1)+"ies";
		else if( !singular.ends_with('s') )
			y = RESULT{ singular }+"s";
		return y;
	}
}
#undef let
#undef BSV
#undef RESULT
#undef YRESULT

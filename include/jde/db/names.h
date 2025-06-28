#pragma once
#include <jde/framework/str.h>

#define BSV Str::bsv<typename T::traits_type>
#define RESULT std::basic_string<char,typename T::traits_type>
#define YRESULT std::basic_string<char,typename Y::traits_type>

namespace Jde::DB::Names{
	α IsPlural( sv name )ι->bool;
	Ξ Capitalize( str name )ι->string{ ASSERT(name.size()>1); return string{(char)std::toupper(name[0])} + name.substr(1); }
	template<class X=string,class Y=string> Ω FromJson( Str::bsv<typename X::traits_type> jsonName )ι->YRESULT;
	α ToJson( str schemaName )ι->string;
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
	Ξ Names::ToSingular( sv plural )ι->string{
		string y{ plural };
		if( plural.ends_with("acl") )
			y = string{ plural.substr(0, plural.size() - 3) } + "ac";
		else if( plural.ends_with("ies") )
			y = string{plural.substr( 0, plural.size()-3 )}+"y";
		else if( plural.ends_with('s') )
			y = plural.substr( 0, plural.size()-1 );
		return y;
	}
	Ŧ Names::ToPlural( BSV singular )ι->RESULT{
		RESULT y{ singular };
		if( singular=="ac" )
			y = "acl";
		if( !IsPlural(singular) ){
			if( singular.ends_with("y") )
				y = RESULT{ singular }.substr(0, singular.size()-1)+"ies";
			else
				y = RESULT{ singular }+"s";
		}
		return y;
	}
}
#undef let
#undef BSV
#undef RESULT
#undef YRESULT
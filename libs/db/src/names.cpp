#include <jde/db/names.h>

#define let const auto
namespace Jde::DB{
	α Names::IsPlural( sv name )ι->bool{ return name.ends_with("ed") || name.ends_with('s') || name=="acl"; }
	α Names::FromJson( sv jsonName )ι->string{
		string sqlName;
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
	α Names::ToJson( str schemaName )ι->string{
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

	α Names::ToSingular( sv plural )ι->string{
		string y{ plural };
		if( plural.ends_with("acl") )
			y = string{ plural.substr(0, plural.size() - 3) } + "ac";
		else if( plural.ends_with("ies") )
			y = string{plural.substr( 0, plural.size()-3 )}+"y";
		else if( plural.ends_with('s') )
			y = plural.substr( 0, plural.size()-1 );
		return y;
	}
	α Names::ToPlural( sv singular )ι->string{
		string y{ singular };
		if( singular=="ac" )
			y = "acl";
		else if( !IsPlural(singular) ){
			y = singular.ends_with("y")
				? string{singular.substr(0, singular.size()-1)}+"ies"
				: string{singular}+"s";
		}
		return y;
	}
}
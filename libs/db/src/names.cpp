#include <jde/db/names.h>

#define let const auto
namespace Jde::DB{
	α Names::IsPlural( sv name )ι->bool{ return name.ends_with("ed") || name.ends_with('s') || name=="acl"; }
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
}
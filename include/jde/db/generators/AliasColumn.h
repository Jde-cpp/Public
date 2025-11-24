#pragma once
#include <jde/db/meta/Column.h>
#include <jde/db/meta/View.h>

namespace Jde::DB{
	struct AliasCol{
		AliasCol( const string& alias, sp<DB::Column> column )ι:Alias{alias}, Column{move(column)}{}
		AliasCol( sp<DB::Column> column )ι:Column{move(column)}{}
   	α operator==( const AliasCol& b )Ι{ return *Column==*b.Column && Alias==b.Alias; }
		α ToString( bool useAlias=true )Ι->string{
			const auto alias = useAlias
				? Alias.empty() && Column->Table ? Column->Table->DBName : Alias
				: "";
			return alias.empty() ? Column->Name : alias+'.'+Column->Name;
		}
		string Alias;
		sp<DB::Column> Column;
	};
}
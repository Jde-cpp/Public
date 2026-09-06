#include "JsonColumn.h"
#include <jde/db/names.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/DBSchema.h>

namespace Jde::QL{
	α JsonColumn::MemberName()Ι->string{
		string y;
		if( Column->IsEnum() && !Column->IsFlags() && Column->Name.ends_with("_id") ){
			y = DB::Names::ToJson( Table().Name );
			if( !Column->IsFlags() )
				y = DB::Names::ToSingular( y );
		}
		else
			y = DB::Names::ToJson( Column->Name );
		return y;
	}
	α JsonColumn::Table()Ι->const DB::Table&{ return Column->IsFlags() || Column->IsEnum() ? *Column->PKTable : *Column->Table; }
}
#pragma once
#include "../exports.h"

namespace Jde::DB{
	struct Column; struct AppSchema; struct Syntax; struct Table;

	struct ΓDB View{
		View( sv name )ι:Name{name}{}  //placeholder columns populated in Initialize
		View( sv name, const jobject& j )ε;
		α Initialize( sp<DB::AppSchema> schema, sp<View> self )ε->void;
		β FindColumn( sv name )Ι->sp<Column>;
		β GetColumn( sv name, SRCE )Ε->const Column&;
		β GetColumnPtr( sv name, SRCE )Ε->sp<Column>;
		β GetColumns( vector<string> names, SRCE )Ε->vector<sp<Column>>;
		α FindPK()Ι->sp<Column>;
		α FindFK( sv pkTableName )Ι->sp<Column>;
		α GetPK( SRCE )Ε->sp<Column>;
		α IsEnum()Ι->bool;
		β IsView()Ι->bool{ return true; }
		α Syntax()Ι->const DB::Syntax&;

		string Name; //provider_id
		vector<sp<Column>> Columns;
		string DBName; //[schema.][um_]Name
		sp<DB::AppSchema> Schema;
		vector<sp<Column>> SurrogateKeys;
	};
	Ξ AsTable(sp<View> v)ι->sp<Table>{ return dynamic_pointer_cast<Table>(v); }
	α AsTable(const View& v)ε->const Table&;
}

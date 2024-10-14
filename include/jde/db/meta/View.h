#pragma once
#include "../exports.h"

namespace Jde::DB{
	struct Column; struct Schema;

	struct ΓDB View{
		View( sv name )ι:Name{name}{}  //placeholder columns populated in Initialize
		View( sv name, const jobject& j )ε;

		β FindColumn( sv name )Ι->sp<Column>;
		β GetColumn( sv name, SRCE )Ε->const Column&;
		β GetColumnPtr( sv name, SRCE )Ε->sp<Column>;
		α FindPK()Ι->sp<Column>;
		α GetPK( SRCE )Ε->sp<Column>;
		β IsView()Ι->bool{ return true; }

		string Name; //provider_id
		vector<sp<Column>> Columns;
		string DBName; //[schema.][um_]Name
		sp<DB::Schema> Schema;
		vector<sp<Column>> SurrogateKeys;
	};
}

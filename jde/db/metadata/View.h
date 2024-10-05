#pragma once
#include "../exports.h"

namespace Jde::DB{
	struct Column; struct Schema;

	struct ΓDB View{
		View( sv name )ι:Name{name}{}  //placeholder columns populated in PopulateParents
		View( sv name, const jobject& j )ε;

		α FindColumn( sv name )Ι->sp<Column>;
		α FindColumnε( sv name, SRCE )Ε->sp<Column>;
		β IsView()Ι->bool{ return true; }
		α SurrogateColumns()Ε->vector<sp<Column>>;

		string Name;
		string DBName; //[schema.][um_]Name
		sp<DB::Schema> Schema;
		vector<string> SurrogateKeys;
		vector<sp<Column>> Columns;
	private:
		string _dbName;
	};
}

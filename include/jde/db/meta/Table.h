#pragma once
#include <jde/db/exports.h>
#include "View.h"

namespace Jde::DB{
	struct AppSchema; struct Column;

	struct ΓDB Table : View{
		//Table( sv schema, sv name )ι:Schema{schema}, Name{name}{}
		Table( sv name )ι:View{name}{}  //placeholder
		Table( sv name, const jobject& j )ε;
		α Initialize( sp<DB::AppSchema> schema, sp<Table> self )ε->void;

		α FindColumn( sv name )Ι->sp<Column> override;
		α GetColumn( sv name, SRCE )Ε->const Column& override;//also looks into extended from table
		α GetColumnPtr( sv name, SRCE )Ε->sp<Column> override;
		α GetColumns( vector<string> names, SRCE )Ε->vector<sp<Column>> override;
		//α GetExtendedFromTable()Ι->sp<View>;//um_users return um_entities
		α NameWithoutType()Ι->sv;//users in um_users.
		α Prefix()Ι->sv;//um in um_users.

		α FKName()Ι->string;
		α IsView()Ι->bool override{ return false; }

		vector<vector<string>> NaturalKeys;
		string PurgeProcName;
		sp<DB::Table> Extends;
	};
	Ξ AsView(sp<Table> t)ι->sp<View>{ return dynamic_pointer_cast<View>(t); }
}
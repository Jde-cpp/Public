#pragma once
#ifndef META_TABLE_H
#define META_TABLE_H
#include <jde/db/exports.h>
#include "View.h"

namespace Jde::DB{
	struct AppSchema; struct Column;

	struct ΓDB Table : View{
		Table( string name )ι:View{move(name)}{}  //placeholder
		Table( sv name, const jobject& j )ε;
		virtual ~Table()=default;
		α Initialize( sp<DB::AppSchema> schema, sp<Table> self )ε->void;

		α FindColumn( sv name )Ι->sp<Column> override;

		α IsView()Ι->bool override{ return false; }

		vector<vector<string>> NaturalKeys;
		string PurgeProcName;
		sp<DB::Table> Extends;
	};
	Ξ AsView(sp<Table> t)ι->sp<View>{ return dynamic_pointer_cast<View>(t); }
}
#endif
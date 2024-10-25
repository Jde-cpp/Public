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

		α InsertProcName()Ι->string;
		α FindColumn( sv name )Ι->sp<Column> override;
		α GetColumn( sv name, SRCE )Ε->const Column& override;//also looks into extended from table
		α GetColumnPtr( sv name, SRCE )Ε->sp<Column> override;
		α GetColumns( vector<string> names, SRCE )Ε->vector<sp<Column>> override;
		α IsEnum()Ι->bool;//GraphQL attribute
		α GetExtendedFromTable()Ι->sp<Table>;//um_users return um_entities
		α NameWithoutType()Ι->sv;//users in um_users.
		α Prefix()Ι->sv;//um in um_users.
		α JsonTypeName()Ι->string;

		α FKName()Ι->string;
		α IsMap()Ι->bool{ return ChildTable() && ParentTable(); }
		α ChildColumn()Ι->sp<Column>{ return ParentChildMap ? get<1>(*ParentChildMap) : sp<Column>{}; }
		α ParentColumn()Ι->sp<Column>{ return ParentChildMap ? get<0>(*ParentChildMap) : sp<Column>{}; }
		α ChildTable()Ι->sp<Table>;
		α ParentTable()Ι->sp<Table>;
		α IsView()Ι->bool override{ return false; }

		bool HaveSequence()Ι;
		bool HasCustomInsertProc{};
		bool IsFlags; //e.g. read=1, update=2, purge=4, execute=8, rights=16
		vector<vector<string>> NaturalKeys;
		optional<tuple<sp<Column>,sp<Column>>> ParentChildMap;//groups: entity_id, member_id
		string PurgeProcName;
		sp<View> QLView;
	};
	Ξ AsView(sp<Table> t)ι->sp<View>{ return dynamic_pointer_cast<View>(t); }
}
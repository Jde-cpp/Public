#pragma once
#include "../DataType.h"
#include <jde/Exports.h>
//#include <jde/Str.h>
#include "View.h"
//#include "Index.h"

namespace Jde::DB{
	//struct Syntax;  struct Table;
	struct Column; struct Schema;

	struct ΓDB Table : View{
		//Table( sv schema, sv name )ι:Schema{schema}, Name{name}{}
		Table( sv name, const jobject& j )ε;
		α Initialize( sp<DB::Schema> schema, sp<Table> self )ε->void;

		α InsertProcName()Ι->string;
		α FindColumnε( sv name )Ε->const Column&;//also looks into extended from table

		α IsFlags()Ι->bool{ return false;/*FlagsData.size()*/; }//TODO!
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
		α Syntax()Ι->const DB::Syntax&;

		bool HaveSequence()Ι;
		bool HasCustomInsertProc{};
		vector<vector<string>> NaturalKeys;
		optional<tuple<sp<Column>,sp<Column>>> ParentChildMap;//groups: entity_id, member_id
		string PurgeProcName;
		sp<DB::Schema> Schema;
		sp<View> QLView;
	};
}
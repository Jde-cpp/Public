#pragma once
#include "../exports.h"
#include <jde/access/usings.h>

namespace Jde::DB{
	struct Column; struct AppSchema; struct Syntax; struct Table;

	struct ΓDB View{
		View( sv name )ι:Name{name},DBName{name}{}  //placeholder columns populated in Initialize
		View( sv name, const jobject& j )ε;
		α Initialize( sp<DB::AppSchema> schema, sp<View> self )ε->void;

		α Authorize( Access::ERights rights, UserPK userPK, SL sl )Ε->void;
		β FindColumn( sv name )Ι->sp<Column>;
		β GetColumn( sv name, SRCE )Ε->const Column&;
		β GetColumnPtr( sv name, SRCE )Ε->sp<Column>;
		β GetColumns( vector<string> names, SRCE )Ε->vector<sp<Column>>;
		α SequenceColumn()Ι->sp<Column>;
		α FindPK()Ι->sp<Column>;
		α FindFK( sv pkTableName )Ι->sp<Column>;
		α GetPK( SRCE )Ε->sp<Column>;
		α GetSK0(SRCE)Ε->sp<Column>;
		α InsertProcName()Ι->string;
		α UpsertProcName()Ι->string;
		α IsEnum()Ι->bool;
		β IsView()Ι->bool{ return true; }
		α JsonName()Ι->string;
		α Syntax()Ι->const DB::Syntax&;

		α ChildTable()Ι->sp<View>;
		α ParentTable()Ι->sp<View>;

		string Name; //provider_id
		vector<sp<Column>> Columns;
		string DBName; //[schema.][um_]Name
		bool HasCustomInsertProc;
		string AddProc;
		string RemoveProc;
		bool IsFlags; //e.g. read=1, update=2, purge=4, execute=8, rights=16
		struct ParentChildMap{ sp<Column> Parent; sp<Column> Child; };
		optional<ParentChildMap> Map;//groups: identity_id, member_id
		sp<View> QLView;
		sp<DB::AppSchema> Schema;
		vector<sp<Column>> SurrogateKeys;
		Access::ERights Operations; //user operations.
		vector<sp<View>> Children;
	};
	α AsTable(sp<View> v)ι->sp<Table>;
	α AsTable(const View& v)ε->const Table&;
}

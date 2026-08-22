#pragma once
#include "../exports.h"
#include <jde/access/usings.h>

#define Φ ΓDB α

namespace Jde::DB{
	struct Column; struct AppSchema; struct Syntax; struct Table;

	struct ΓDB View{
		View( string name )ι:Name{name},DBName{move(name)}{}  //placeholder columns populated in Initialize
		View( sv name, const jobject& j )ε;
		virtual ~View()=default;
		α Initialize( sp<DB::AppSchema> schema, sp<View> self )ε->void;

		α Authorize( Access::ERights rights, UserPK userPK, SL sl )Ε->void;
		β FindColumn( sv name )Ι->sp<Column>;
		β GetColumn( sv name, SRCE )Ε->const Column&;
		β GetColumnPtr( sv name, SRCE )Ε->sp<Column>;
		β GetColumns( vector<string> names, SRCE )Ε->vector<sp<Column>>;
		α FindPK()Ι->sp<Column>;
		α FindFK( sv pkTableName )Ι->sp<Column>;
		α GetPK( SRCE )Ε->sp<Column>;
		α GetSK0(SRCE)Ε->sp<Column>;
		α InsertProcName()Ι->string;
		α DdlInsertProcName()Ι->string; //InsertProcName, but empty when there is no server object to create/drop.
		α UpsertProcName()Ι->string;
		α IsEnum()Ι->bool;
		β IsView()Ι->bool{ return true; }
		α JsonName()Ι->string;
		α SequenceColumn()Ι->sp<Column>;
		α SqlName()Ι->string; //DBName, quoted when it is a reserved word (e.g. an unprefixed `groups`) - DBName stays raw for metadata lookups.
		α Syntax()Ι->const DB::Syntax&;


		string Name; //provider_id
		vector<sp<Column>> Columns;
		string DBName; //[schema.][um_]Name
		bool HasCustomInsertProc;
		string AddProc;
		string RemoveProc;
		bool IsFlags; //e.g. read=1, update=2, purge=4, execute=8, rights=16
		vector<sp<Column>> SurrogateKeys;//before Map
		struct ParentChildMap{ sp<Column> Parent; sp<Column> Child; };
		optional<ParentChildMap> Map;//members: identity_id, member_id
		sp<View> QLView;
		//The table that declared this one as its qlView, set in View::Initialize.  Weak, or the pair would keep each other
		//alive.  Authorize tests the owner's name:  a resource exists per table ("users"), never per ql view ("usersQl").
		wp<View> Owner;
		sp<DB::AppSchema> Schema;
		Access::ERights Operations; //user operations.
		vector<sp<View>> Children;
	};
	Φ AsTable(sp<View> v)ι->sp<Table>;
	Φ AsTable(const View& v)ε->const Table&;
}
#undef Φ
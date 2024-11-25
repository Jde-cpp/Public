#pragma once

namespace Jde::DB{
	struct Column; struct Table; struct View; struct WhereClause;

	struct Join final{
		//JoinTable( sp<DB::View> view )ι:View{view}{}
		//Join( sp<Column> from, sp<Column> to, bool inner )ι:From{from},To{to},Inner{inner}{}
		sp<Column> From;
		sp<Column> To;
		bool Inner{};
		string ToAlias;
	};
	struct FromClause final{
		FromClause()=default;
		FromClause( sp<View>& v )ι:SingleTable{v}{};
		FromClause( const sp<Table>& t )ι;
		FromClause( vec<sp<Table>>& tables, SRCE )ε;
		FromClause( Join&& j )ι:Joins{move(j)}{};
		α ToString()Ε->string;
		α operator+=( Join&& join )ι->FromClause&;
		α TryAdd( Join&& join )ι->void;

		α Contains( sv tableName )ι->bool;
		α Empty()Ι->bool{ return !SingleTable && Joins.empty(); }
		α GetColumnPtr( sv name, SL sl )Ε->sp<Column>;
		α GetFirstTable( SL sl )Ε->sp<View>;
		α SetActive( WhereClause& where, SRCE )ε->void;
		sp<View> SingleTable;
		vector<Join> Joins;
	};
}

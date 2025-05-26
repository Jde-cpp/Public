#pragma once
#include "../exports.h"

namespace Jde::DB{
	struct Column; struct Table; struct View; struct WhereClause;

	struct Join final{
		sp<Column> From;
		sp<Column> To;
		bool Inner{};
		string ToAlias;
		string FromAlias;
	};
	struct ΓDB FromClause final{
		FromClause()=default;
		FromClause( const sp<View>& v )ι;
		FromClause( const sp<Table>& t )ε;
		FromClause( vec<sp<Table>>& tables, SRCE )ε;
		FromClause( Join&& j )ι:Joins{move(j)}{};
		α ToString()Ε->string;
		α operator+=( Join&& join )ι->FromClause&;
		α Add( sp<Column> from, sp<Column> to, bool inner=true )ι->void;
		α TryAdd( Join&& join )ι->void;

		α Contains( sv tableName )ι->bool;
		α Empty()Ι->bool{ return Joins.empty(); }
		α GetColumnPtr( sv name, SL sl )Ε->sp<Column>;
		α GetFirstTable( SRCE )Ε->sp<View>;
		α SetActive( WhereClause& where, SRCE )ε->void;
		vector<Join> Joins;
	};
}
#pragma once

namespace Jde::DB{
	struct Column; struct Table; struct View; struct WhereClause;

	struct Join final{
		//JoinTable( sp<DB::View> view )ι:View{view}{}
		Join( sp<Column> from, sp<Column> to )ι:From{from},To{to}{}
		sp<Column> From;
		sp<Column> To;
	};
	struct FromClause final{
		FromClause( sp<View>& v )ι:SingleTable{v}{};
		FromClause( sp<Table>& t )ι:SingleTable{std::dynamic_pointer_cast<View>(t)}{};
		FromClause( vec<sp<Table>>& tables, SRCE )ε;
		FromClause( Join&& j )ι:Joins{move(j)}{};
		α ToString()Ε->string;
		α operator+=( Join&& join )ι->FromClause&;

		α GetColumnPtr( sv name, SL sl )Ε->sp<Column>;
		α GetFirstTable( SL sl )Ε->sp<View>;
		α SetActive( WhereClause& where, SRCE )ε->void;
		sp<View> SingleTable;
		vector<Join> Joins;
	};
}

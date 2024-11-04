#pragma once

namespace Jde::DB{
	struct Column; struct FromClause; struct JoinClause; struct Table; struct WhereClause;
	struct SelectClause final{
		SelectClause()=default;
		SelectClause( sp<Column> c )ι:Columns{c}{}
		α TryAdd( sp<DB::Column> c )ι->void;
		vector<sp<DB::Column>> Columns;
		α ToString()Ι->string;
		α FindColumn( sv name )Ι->sp<Column>;
	private:
		α FindColumn( const DB::Column& c )Ι->sp<Column>;
	};
}
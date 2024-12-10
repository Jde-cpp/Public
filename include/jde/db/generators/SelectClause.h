#pragma once

namespace Jde::DB{
	struct Column;
	struct SelectClause final{
		SelectClause()=default;
		SelectClause( sp<Column> c )ι:Columns{c}{}
		SelectClause( vector<sp<Column>> cols )ι:Columns{move(cols)}{}//{ Columns.
		α operator+=( SelectClause&& x )ι->SelectClause&;
		α TryAdd( sp<DB::Column> c )ι->void;
		α ToString()Ι->string;
		α FindColumn( sv name )Ι->sp<Column>;

		vector<sp<DB::Column>> Columns;
	private:
		α FindColumn( const DB::Column& c )Ι->sp<Column>;
	};
}
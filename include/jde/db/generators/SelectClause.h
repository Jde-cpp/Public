#pragma once
#include "../exports.h"
#include "AliasColumn.h"

namespace Jde::DB{
	struct Column;
	struct ΓDB SelectClause final{
		SelectClause()=default;
		SelectClause( sp<Column> c )ι:Columns{{{},c}}{}
		SelectClause( const View& t, const vector<string>& cols )ι;
		SelectClause( AliasCol aliasCol )ι:Columns{aliasCol}{}
		SelectClause( const vector<sp<Column>>& cols, const string& alias={} )ι;
		α operator+=( SelectClause&& x )ι->SelectClause&;
		α TryAdd( sp<DB::Column> c )ι->void;
		α TryAdd( const AliasCol& c )ι->void;
		α ToString( bool shouldAlias )Ι->string;
		α FindColumn( sv name )Ι->sp<Column>;
		α FindColumn( const AliasCol& c )Ι->sp<Column>;
		vector<AliasCol> Columns;
	private:
		α FindColumn( const DB::Column& c )Ι->sp<Column>;
	};
}
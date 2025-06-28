#pragma once
#include "../exports.h"
#include "AliasColumn.h"
#include "Object.h"

namespace Jde::DB{
	struct Column;
	struct ΓDB SelectClause final{
		SelectClause()=default;
		SelectClause( sp<Column> c )ι;
		SelectClause( const View& t, str alias, const vector<string>& cols )ι;
		SelectClause( AliasCol aliasCol )ι;
		SelectClause( const vector<sp<Column>>& cols, const string& alias={} )ι;
		SelectClause( const DB::Object& obj )ι;
		α operator+=( SelectClause&& x )ι->SelectClause&;
		α TryAdd( Object c )ι->void;
		α TryAdd( const AliasCol& c )ι->void;
		α TryAdd( const sp<Column>& c )ι->void;
		α ToString( bool shouldAlias )Ι->string;
		α FindColumn( sv name )Ι->sp<Column>;
		α FindColumn( const Object& c )Ι->const Object*;
		vector<Object> Columns;
	private:
		α FindColumn( const DB::Column& c )Ι->sp<Column>;
	};
}
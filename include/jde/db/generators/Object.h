#pragma once
#include <jde/db/meta/Column.h>
#include <jde/db/generators/AliasColumn.h>
#include <jde/db/Value.h>

namespace Jde::DB{
	enum class EObject : uint{ Value, Values, AliasColumn, Coalesce, Count };
	struct Coalesce; struct Count;
	using Object=variant<Value, vector<Value>, AliasCol, Coalesce, Count>;

  α operator==(const Object& a, const Object& b)ι->bool;

	α ToString( const Object& o )ι->string;
	α GetParams( Object& o )ι->vector<Value>;
	α GetParams( const Object& o )ι->vector<Value>;
}
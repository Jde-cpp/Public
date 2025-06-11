#pragma once
#include <jde/db/meta/Column.h>
#include <jde/db/generators/AliasColumn.h>
#include <jde/db/Value.h>
//#include <jde/db/generators/Coalesce.h>

namespace Jde::DB{
	enum class EObject : uint{ Value, Values, Column, AliasColumn, Coalesce };
	struct Coalesce;
	using Object=variant<Value, vector<Value>, sp<Column>, AliasCol, Coalesce>;

	α ToString( Object& o )ι->string;
	α GetParams( Object& o )ι->vector<Value>;
}
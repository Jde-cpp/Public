#pragma once
#include <jde/db/meta/Column.h>
#include <jde/db/generators/AliasColumn.h>
#include <jde/db/Value.h>
#include <jde/db/exports.h>

namespace Jde::DB{
	enum class EObject : uint{ Value, Values, AliasColumn, Coalesce, Count };

	struct Count final{
		Count()ι{};
		α ToString()Ι->string{ return "count(*)"; }
		α Params()ι->vector<Value>{ return {}; }
	};

	struct Coalesce;
	using Object=variant<Value, vector<Value>, AliasCol, Coalesce, Count>;

	struct ΓDB Coalesce final{
		Coalesce( Object&& a, Object&& b )ι;
		α ToString()Ι->string;
		α Params()ι->vector<Value>;
		α Params()Ι->vector<Value>;
		vector<Object> Objects;
	};

  α operator==(const Object& a, const Object& b)ι->bool;

	α ToString( const Object& o )ι->string;
	α GetParams( Object& o )ι->vector<Value>;
	α GetParams( const Object& o )ι->vector<Value>;
}
#pragma once
#include "../Value.h"
#include "../meta/Column.h"
#include "Object.h"
#include <jde/db/exports.h>

namespace Jde::DB{
	struct ΓDB Coalesce final{
		Coalesce( Object&& a, Object&& b )ι;
		α ToString()Ι->string;
		α Params()ι->vector<Value>;
		α Params()Ι->vector<Value>;
		vector<Object> Objects;
	};
	struct Count final{
		Count()ι;
		α ToString()Ι->string{ return "count(*)"; }
		α Params()ι->vector<Value>{return {};}
	};
}
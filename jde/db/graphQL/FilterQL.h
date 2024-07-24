#pragma once
#include "../../../../Framework/source/db/DataType.h"

namespace Jde::DB{
	enum class EQLOperator : uint8{
		Equal,
		NotEqual,
		Regex,
		Glob,
		In,
		NotIn,
		GreaterThan,
		GreaterThanOrEqual,
		LessThan,
		LessThanOrEqual,
		ElementMatch
	};
	inline constexpr std::array<sv,11> EQLOperatorStrings = { "eq", "ne", "regex", "glob", "in", "nin", "gt", "gte", "lt", "lte", "elemMatch" };
	struct FilterValueQL final{
		EQLOperator Operator;
		json Value;
		α Test( const DB::object& value, ELogTags logTags )Ι->bool;
	};
	using JsonColumnName=string;
	struct FilterQL final{//filter:
		flat_map<JsonColumnName,vector<FilterValueQL>> ColumnFilters;
		Ω Test( const DB::object& value, const vector<FilterValueQL>& filters, ELogTags logTags )ι->bool;
	};
}
#pragma once

namespace Jde::DB{
	struct Table;
	struct ForeignKey final{
		Ω Create( sv name, sv columnName, const DB::Table& pk, const DB::Table& foreignTable )ε->string;

		string Name;
		string Table;
		vector<string> Columns;
		string pkTable;
	};
}
#pragma once

namespace Jde::DB{
	struct View;
	struct ForeignKey final{
		Ω Create( sv name, sv columnName, const DB::View& pk, const DB::View& foreignTable )ε->string;

		string Name;
		string Table;
		vector<string> Columns;
		string pkTable;
	};
}
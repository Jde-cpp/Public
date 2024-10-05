#pragma once

namespace Jde::DB{
	struct Cluster; struct IDataSource; struct Schema; struct Syntax;

	//Cluster > Catalog > Schema > Table > Columns & Rows
	//sql server=Database.
	//mysql=n/a ie single catalog.
	struct Catalog final{
		Catalog( sv name, const jobject& config )ε;

		Ω Initialize( sp<DB::Cluster> cluster, sp<Catalog> self )ι->void;
		α DS()ε->sp<IDataSource>;
		α Syntax()Ι->const Syntax&;
		string Name;
		string DBName;
		sp<DB::Cluster> Cluster;
		vector<sp<DB::Schema>> Schemas;
	private:
		sp<IDataSource> _dataSource;
	};
}

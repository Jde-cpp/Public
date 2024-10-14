#pragma once

namespace Jde::DB{
	struct Cluster; struct IDataSource; struct Schema; struct Syntax;

	//Cluster > Catalog > Schema > Table > Columns & Rows
	//sql server=Database.
	//mysql=n/a ie single catalog.
	struct Catalog final{
		Catalog( sv name, const jobject& config )ε;

		Ω Initialize( sp<DB::Cluster> cluster, sp<Catalog> self )ι->void;
		α DS()Ε->sp<IDataSource>;
		α Syntax()Ι->const Syntax&;
		α FindSchema( sv name )ι->sp<Schema>;
		string Name;
		sp<DB::Cluster> Cluster;
		vector<sp<Schema>> Schemas;
	private:
		mutable sp<IDataSource> _dataSource;
	};
}

#pragma once


namespace Jde::DB{
	//Cluster > Catalog > Schema > Table > Columns & Rows
	//sql server, mysql = service instance.
	struct Catalog; struct IDataSource; struct Schema; struct Syntax;
	struct Cluster final{
		Cluster( sv name, const jobject& config )ε;

		Ω Initialize( sp<Cluster> cluster )ι->void;
		α Syntax()Ι->const DB::Syntax&;
		α GetSchema( sv name, SRCE )ε->sp<Schema>;

		string ConfigName;
		const vector<sp<Catalog>> Catalogs;
		sp<IDataSource> DataSource;
		bool ShouldPrefixTable;
	};
}

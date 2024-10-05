#pragma once

namespace Jde::DB{
	//Cluster > Catalog > Schema > Table > Columns & Rows
	//sql server, mysql = service instance.
	struct Catalog; struct IDataSource; struct Syntax;
	struct Cluster final{
		Cluster( const jobject& config )ε;
		Ω Initialize( sp<Cluster> cluster )ι;

		const vector<sp<Catalog>> Catalogs;
		sp<IDataSource> DataSource;
		bool ShouldPrefixTable;
	};
}

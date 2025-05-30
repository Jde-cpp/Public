#pragma once

namespace Jde::Access{ struct IAcl; }
namespace Jde::DB{
	//Cluster > Catalog > Schema > Table > Columns & Rows
	//sql server, mysql = service instance.
	struct Catalog; struct IDataSource; struct AppSchema; struct Syntax;
	struct Cluster final{
		Cluster( sv name, const jobject& config, sp<Access::IAcl> authorizer )ε;

		Ω Initialize( sp<Cluster> cluster )ε->void;
		α Syntax()Ι->const DB::Syntax&;
		α GetAppSchema( str name, SRCE )ε->sp<AppSchema>;

		string ConfigName;
		const vector<sp<Catalog>> Catalogs;
		sp<IDataSource> DataSource;
	};
}

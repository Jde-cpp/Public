#pragma once
#include "Table.h"
#include <jde/framework/str.h>
#include <jde/framework/io/json.h>

namespace Jde::Access{ struct IAcl; }
namespace Jde::DB{
	struct AppSchema; struct Catalog; struct IDataSource; struct View; struct Syntax;
	//Cluster > Catalog > Schema > Table > Columns & Rows
	struct DBSchema{
		DBSchema( sv name, const jobject& DBSchema, sp<Access::IAcl> authorizer )ε;
		DBSchema( sv name, flat_map<string,sp<Table>> tables, sv prefix )ι;
		α ResetDS()Ι{ _dataSource = nullptr; }
		α DS()Ε->sp<IDataSource>;

		α FindAppSchema( str name )ι->sp<AppSchema>;
		α FindDefTable( const Table& t1, const Table& t2 )Ι->sp<Table>;//route, route_steps-> -> route_definition
		Ω Initialize( sp<DB::Catalog> catalog, sp<DBSchema> self )ε->void;
		α IsPhysical()Ι->bool{ return Name.size() && Name[0]!='_'; }

		const string Name; //empty=default, acutal name ie [jde_test].
		sp<DB::Catalog> Catalog;
		flat_map<string,sp<AppSchema>> AppSchemas;
	private:
		mutable sp<IDataSource> _dataSource;
	};
}

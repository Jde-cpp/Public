#pragma once
//#ifndef SCHEMA_H
//#define SCHEMA_H
#include "Table.h"
#include <jde/framework/str.h>
#include <jde/framework/io/json.h>

namespace Jde::Access{ struct IAcl; }
namespace Jde::DB{
	struct AppSchema; struct Catalog; struct IDataSource; struct View; struct Syntax;
	//Cluster > Catalog > Schema > Table > Columns & Rows
	struct DBSchema{
		DBSchema( sv name, const jobject& DBSchema, sp<Access::IAcl> authorizer )ε;
		DBSchema( sv name, flat_map<string,sp<Table>> tables )ι;
		α ResetDS()Ι{ _dataSource = nullptr; }
		α DS()Ε->sp<IDataSource>;

		α FindAppSchema( str name )ι->sp<AppSchema>;
		//α FindTableSuffix( sv suffix, SRCE )Ε->const Table&;// um_users find users.
		//α TryFindTableSuffix( sv suffix )Ι->sp<Table>;
		α FindDefTable( const Table& t1, const Table& t2 )Ι->sp<Table>;//route, route_steps-> -> route_definition
		Ω Initialize( sp<DB::Catalog> catalog, sp<DBSchema> self )ε->void;

		const string Name; //empty=default, acutal name ie [jde_test].
		sp<DB::Catalog> Catalog;
		flat_map<string,sp<AppSchema>> AppSchemas;
	private:
		mutable sp<IDataSource> _dataSource;
	};

/*	Ξ DBSchema::TryFindTableSuffix( sv suffix )Ι->sp<Table>{
		sp<Table> y;
		for( let& [name,pTable] : Tables ){
			if( name.ends_with(suffix) && name.size()>suffix.size()+2 && name[name.size()-suffix.size()-1]=='_' && name.substr(0,name.size()-suffix.size()-1).find_first_of('_')==string::npos ){
				y = pTable;
				break;
			}
		}
		return y;
	}
	Ξ DBSchema::FindTableSuffix( sv suffix, SL sl )Ε->const Table&{
		let y = TryFindTableSuffix( suffix );
		if( !y ) throw Exception{ sl, ELogLevel::Debug, "Could not find table '{}' in schema", suffix };//mysql can't use THROW_IF
		return *y;
	}
*/
	// route, route_steps -> route_definition

}
//#endif
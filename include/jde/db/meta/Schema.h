#pragma once
#ifndef SCHEMA_H
#define SCHEMA_H
#include "Table.h"
#include <jde/framework/str.h>
#include <jde/framework/io/json.h>

#define BSV Str::bsv<typename T::traits_type>
#define RESULT std::basic_string<char,typename T::traits_type>
#define YRESULT std::basic_string<char,typename Y::traits_type>
namespace Jde::DB{
	struct Catalog; struct IDataSource; struct View; struct Syntax;
	//Cluster > Catalog > Schema > Table > Columns & Rows
	struct Schema{
		Schema( sv name, const jobject& schema )ε;
		Schema( sv name, flat_map<string,sp<Table>> tables )ι;
		α DS()Ε->sp<IDataSource>;

		α FindView( str name )Ι->sp<View>;
		α GetView( str name, SRCE )ε->const View&;
		α GetViewPtr( str name, SRCE )ε->sp<View>;

		α FindTable( str name )Ι->sp<Table>;
		α GetTable( str name, SRCE )Ε->const Table&;
		α GetTablePtr( str name, SRCE )Ε->sp<Table>;

		//α FindTableSuffix( sv suffix, SRCE )Ε->const Table&;// um_users find users.
		//α TryFindTableSuffix( sv suffix )Ι->sp<Table>;
		α FindDefTable( const Table& t1, const Table& t2 )Ι->sp<Table>;//route, route_steps-> -> route_definition
		Ω Initialize( sp<DB::Catalog> catalog, sp<Schema> self )ι->void;
		template<class X=string,class Y=string> Ω FromJson( Str::bsv<typename X::traits_type> jsonName )ι->YRESULT;
		template<class X=string,class Y=string> Ω ToJson( Str::bsv<typename X::traits_type> schemaName )ι->YRESULT;
		Ω ToSingular( sv plural )ι->string;
		α Syntax()Ι->const Syntax&;
		template<class T=string> Ω ToPlural( BSV singular )ι->RESULT;

		const string Name; //config name um,logs, iot.
		sp<DB::Catalog> Catalog;
		string DBName; //empty=default, acutal name ie [test_um].
		flat_map<string,sp<Table>> Tables;
		flat_map<string,sp<View>> Views;
	private:
		mutable sp<IDataSource> _dataSource;
	};
#define let const auto
	template<class X,class Y> α Schema::FromJson( Str::bsv<typename X::traits_type> jsonName )ι->YRESULT{
		YRESULT sqlName; sqlName.reserve( jsonName.size() );
		for( let ch : jsonName ){
			if( std::isupper(ch) ){
				sqlName+="_";
				sqlName +=(char)std::tolower( ch );
			}
			else
				sqlName+=ch;
		}
		return sqlName;
	}
	template<class X,class Y> α Schema::ToJson( Str::bsv<typename X::traits_type> schemaName )ι->YRESULT{
		std::ostringstream j;
		bool upper = false;
		for( let ch : schemaName ){
			if( ch=='_' )
				upper = true;
			else if( upper ){
				j << (char)std::toupper( ch );
				upper = false;
			}
			else if( j.tellp()==0 )
				j << (char)tolower( ch );
			else
				j << ch;
		}
		return j.str();
	}
	Ξ Schema::ToSingular( sv plural )ι->string{
		string y{ plural };
		if( plural.ends_with("ies") )
			y = string{plural.substr( 0, plural.size()-3 )}+"y";
		else if( plural.ends_with('s') )
			y = plural.substr( 0, plural.size()-1 );
		return y;
	}
	Ŧ Schema::ToPlural( BSV singular )ι->RESULT{
		RESULT y{ singular };
		if( singular.ends_with("y") )
			y = RESULT{ singular }.substr(0, singular.size()-1)+"ies";
		else if( !singular.ends_with('s') )
			y = RESULT{ singular }+"s";
		return y;
	}

/*	Ξ Schema::TryFindTableSuffix( sv suffix )Ι->sp<Table>{
		sp<Table> y;
		for( let& [name,pTable] : Tables ){
			if( name.ends_with(suffix) && name.size()>suffix.size()+2 && name[name.size()-suffix.size()-1]=='_' && name.substr(0,name.size()-suffix.size()-1).find_first_of('_')==string::npos ){
				y = pTable;
				break;
			}
		}
		return y;
	}
	Ξ Schema::FindTableSuffix( sv suffix, SL sl )Ε->const Table&{
		let y = TryFindTableSuffix( suffix );
		if( !y ) throw Exception{ sl, ELogLevel::Debug, "Could not find table '{}' in schema", suffix };//mysql can't use THROW_IF
		return *y;
	}
*/
	// route, route_steps -> route_definition
#undef let
#undef BSV
#undef RESULT
#undef YRESULT
}
#endif
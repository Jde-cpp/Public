#include "CatalogDdl.h"
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/generators/Syntax.h>
#include <jde/db/meta/Catalog.h>
#include <jde/db/meta/AppSchema.h>
#include "SchemaDdl.h"

#define let const auto

namespace Jde::DB{

#ifndef PROD
namespace CatalogDdl{
	α SysDS( sp<IDataSource> ds )ι->sp<IDataSource>{
		return ds->Syntax().HasCatalogs()
			? ds->AtCatalog( "master" )
			: ds;
	}
	Ω recreateCatalog( IDataSource& ds, sv catalog )ε->void{
		ds.ExecuteSync( {Ƒ("use master; DROP DATABASE IF EXISTS {0};create database {0};", catalog)} );
	}

	α NonProd::Drop( const AppSchema& schema )ε->void{
		auto ds = schema.DS();
		if( ds->Syntax().HasCatalogs() ){
			let catalogName = schema.DBSchema->Catalog->Name;
			if( ds->CatalogName()==catalogName )
				ds = SysDS( ds );
			recreateCatalog( *ds, catalogName );
			schema.DBSchema->Catalog->SetDS( ds->AtCatalog(catalogName) );
		}
		else
			SchemaDdl::Drop( schema );
	}
#endif
}}
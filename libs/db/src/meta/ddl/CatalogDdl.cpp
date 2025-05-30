#include "CatalogDdl.h"
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Syntax.h>
#include <jde/db/meta/Catalog.h>
#include <jde/db/meta/AppSchema.h>
#include "SchemaDdl.h"

#define let const auto

namespace Jde::DB{
	α CatalogDdl::Create( IDataSource& ds, sv catalog )ε->void{
		ds.Execute( Ƒ("create database {}", catalog) );
	}

#ifndef PROD
namespace CatalogDdl{
	α SysDS( sp<IDataSource> ds )ι->sp<IDataSource>{
		return ds->Syntax().HasCatalogs()
			? ds->AtCatalog( "master" )
			: ds;
	}
	α DropCatalog( IDataSource& ds, sv catalog )ε->void{
		ds.Execute( Ƒ("DROP DATABASE IF EXISTS {}", catalog) );
	}

	α NonProd::Drop( const AppSchema& schema )ε->void{
		auto ds = schema.DS();
		if( ds->Syntax().HasCatalogs() ){
			let catalogName = schema.DBSchema->Catalog->Name;
			if( ds->CatalogName()==catalogName )
				ds = SysDS( ds );
			DropCatalog( *ds, catalogName );
			Create( *ds, catalogName );
			schema.DBSchema->Catalog->SetDS( ds->AtCatalog(catalogName) );
		}
		SchemaDdl::Drop( schema );
	}
#endif
}}
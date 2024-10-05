#include <jde/db/metadata/Catalog.h>
#include "jde/db/metadata/Cluster.h"
#include <jde/db/metadata/Schema.h>
#include <jde/db/DataSource.h>
#include "ddl/Syntax.h"

namespace Jde::DB{
	α GetSchemas( const jobject& config )ε->vector<sp<Schema>>{
		vector<sp<Schema>> schemas;
    for( auto&& [name, value] : config )
			schemas.emplace_back( ms<Schema>( name, value.as_object() ) );

		return schemas;
	}

	Catalog::Catalog( sv name, const jobject& config )ε:
		Name{ name=="_" ? "" : name },
		Schemas{ GetSchemas(config) }
	{}

	α Catalog::Initialize( sp<DB::Cluster> cluster, sp<Catalog> self )ι->void{
		self->Cluster=cluster;
		for_each(self->Schemas, [self](auto&& schema){ schema->Initialize(self,schema); });
	}

	α Catalog::DS()ε->sp<IDataSource>{
		if( !_dataSource ){
			_dataSource = DBName.empty() || Cluster->DataSource->CatalogName()==DBName
				? Cluster->DataSource
				: Cluster->DataSource->AtCatalog( DBName );
		}
		return _dataSource;
	}

	α Catalog::Syntax()Ι->const DB::Syntax&{ return *Cluster->Syntax; }
}
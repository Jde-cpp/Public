#include <jde/db/meta/Catalog.h>
#include <jde/db/meta/Cluster.h>
#include <jde/db/meta/Schema.h>
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Syntax.h>

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

	α Catalog::DS()Ε->sp<IDataSource>{
		if( !_dataSource ){
			auto cluster = Cluster->DataSource;
			_dataSource = !cluster->Syntax().HasCatalogs() || Name.empty() || cluster->CatalogName()==Name
				? cluster
				: cluster->AtCatalog( Name );
		}
		return _dataSource;
	}
	α Catalog::FindSchema( sv name )ι->sp<Schema>{
		auto p = find_if( Schemas, [name](const sp<Schema>& schema){ return schema->Name==name;} );
		return p==Schemas.end() ? nullptr : *p;
	}

	α Catalog::Syntax()Ι->const DB::Syntax&{ return Cluster->Syntax(); }
}
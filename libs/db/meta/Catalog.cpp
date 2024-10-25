#include <jde/db/meta/Catalog.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Cluster.h>
#include <jde/db/meta/DBSchema.h>
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Syntax.h>

#define let const auto

namespace Jde::DB{
	α GetSchemas( const jobject& config, sp<Access::IAcl> authorizer )ε->vector<sp<DBSchema>>{
		vector<sp<DBSchema>> schemas;
    for( auto&& [name, value] : config )
			schemas.emplace_back( ms<DBSchema>( name, Json::AsObject(value), authorizer ) );
		return schemas;
	}

	Catalog::Catalog( sv name, const jobject& config, sp<Access::IAcl> authorizer )ε:
		Name{ name=="_" ? "" : name },
		Schemas{ GetSchemas(Json::AsObject(config, "schemas"), authorizer) }
	{}

	α Catalog::Initialize( sp<DB::Cluster> cluster, sp<Catalog> self )ε->void{
		ASSERT( self->Cluster==nullptr );
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
	α Catalog::FindAppSchema( str name )ι->sp<AppSchema>{
		sp<AppSchema> pSchema;
		for( let& dbSchema : Schemas ){
			if( pSchema = dbSchema->FindAppSchema(name); pSchema )
				break;
		}
		return pSchema;
	}
}
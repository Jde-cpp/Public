#include <jde/db/meta/Cluster.h>
#include <jde/framework/io/json.h>
#include <jde/db/meta/Catalog.h>
#include <jde/db/Database.h>
#include <jde/framework/io/file.h>
#include <jde/db/generators/Syntax.h>
#include <jde/db/db.h>

#define let const auto

namespace Jde::DB{
	α GetCatalogs( const jobject& catalogs, sp<Access::IAcl> authorizer )ε->vector<sp<Catalog>>{
		vector<sp<Catalog>> y;
		for( let& [name,catalog] : catalogs )
			y.emplace_back( ms<Catalog>(name, Json::AsObject(catalog), authorizer) );
		return y;
	}

	Cluster::Cluster( sv name, const jobject& config, sp<Access::IAcl> authorizer )ε:
		ConfigName{ name },
		Catalogs{ GetCatalogs( Json::AsObject(config, "catalogs"), authorizer) },
		DataSource{ DB::DataSource( Json::AsString(config, "driver"), Json::AsSV(config,"connectionString")) }
	{}

	α Cluster::Initialize( sp<Cluster> self )ε->void{
		for_each(self->Catalogs, [self](auto&& catalog){ catalog->Initialize(self,catalog); });
	}

	α Cluster::GetAppSchema( str name, SL sl )ε->sp<AppSchema>{
		sp<AppSchema> pSchema;
		for( let& catalog : Catalogs ){
			if( pSchema = catalog->FindAppSchema(name); pSchema )
				break;
		}
		THROW_IFSL( !pSchema, "Schema '{}' not found.", name );
		return pSchema;
	}

	α Cluster::Syntax()Ι->const DB::Syntax&{ return DataSource->Syntax(); };

}
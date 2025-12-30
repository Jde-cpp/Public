#include <jde/db/meta/Cluster.h>
#include <jde/fwk/io/json.h>
#include <jde/fwk/io/file.h>
#include <jde/db/db.h>
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/generators/Syntax.h>
#include <jde/db/meta/Catalog.h>

#define let const auto

namespace Jde::DB{
	Ω getCatalogs( const jobject& catalogs, sp<Access::IAcl> authorizer )ε->vector<sp<Catalog>>{
		vector<sp<Catalog>> y;
		for( let& [name,catalog] : catalogs )
			y.emplace_back( ms<Catalog>(name, catalog.get_object(), authorizer) );
		return y;
	}

	Cluster::Cluster( sv name, const jobject& config, sp<Access::IAcl> authorizer )ε:
		ConfigName{ name },
		Catalogs{ getCatalogs( Json::AsObject(config, "catalogs"), authorizer) },
		DataSource{ DB::DataSource(config) }
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
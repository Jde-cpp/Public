#include <jde/db/meta/Cluster.h>
#include <jde/db/meta/Catalog.h>
#include <jde/db/Database.h>
#include <jde/framework/io/File.h>
#include <jde/db/generators/Syntax.h>

#define let const auto

namespace Jde::DB{
	α GetCatalogs( const jarray& metaDataFiles )ε->vector<sp<Catalog>>{
		vector<sp<Catalog>> catalogs;
		for( let& file : metaDataFiles ){
			fs::path path{ string{file.as_string()} };
			jobject config{ parse(IO::FileUtilities::Load(path)).as_object() };
			for( let& [name,catalog] : config )
				catalogs.emplace_back( ms<Catalog>(name, catalog.as_object()) );
		}
		return catalogs;
	}

	Cluster::Cluster( sv name, const jobject& config )ε:
		ConfigName{ config.at("name").as_string() },
		Catalogs{ GetCatalogs(config.at("meta").as_array()) },
		DataSource{ DB::DataSource( fs::path{string{config.at("driver").as_string()}}, config.at("connectionString").as_string()) },
		ShouldPrefixTable{ config.contains("shouldPrefixTable") ? config.at("shouldPrefixTable").as_bool() : false }
	{}

	α Cluster::Initialize( sp<Cluster> self )ι->void{
		for_each(self->Catalogs, [self](auto&& catalog){ catalog->Initialize(self,catalog); });
	}

	α Cluster::GetSchema( sv name, SL sl )ε->sp<Schema>{
		sp<Schema> pSchema;
		for( let& catalog : Catalogs ){
			if( pSchema = catalog->FindSchema(name); pSchema )
				break;
		}
		THROW_IFSL( !pSchema, "Schema '{}' not found.", name );
		return pSchema;
	}

	α Cluster::Syntax()Ι->const DB::Syntax&{ return DataSource->Syntax(); };

}
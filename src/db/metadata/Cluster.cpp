#include <jde/db/metadata/Cluster.h>
#include <jde/db/metadata/Catalog.h>
#include <jde/db/Database.h>
#include <jde/io/File.h>
#include "ddl/Syntax.h"

#define var const auto

namespace Jde::DB{
	α GetCatalogs( const jarray& metaDataFiles )ε->vector<sp<Catalog>>{
		vector<sp<Catalog>> catalogs;
		for( var& file : metaDataFiles ){
			fs::path path{ string{file.as_string()} };
			jobject config{ parse(IO::FileUtilities::Load(path)).as_object() };
			for( var& [name,catalog] : config )
				catalogs.emplace_back( ms<Catalog>(name, catalog.as_object()) );
		}
		return catalogs;
	}

	Cluster::Cluster( const jobject& config )ε:
		Catalogs{ GetCatalogs(config.at("metadata").as_array()) },
		DataSource{ DB::DataSource( fs::path{string{config.at("driver").as_string()}}, config.at("connectionString").as_string()) },
		ShouldPrefixTable{ config.contains("shouldPrefixTable") ? config.at("shouldPrefixTable").as_bool() : false }
	{}

	α Cluster::Initialize( sp<Cluster> self )ι{
		for_each(self->Catalogs, [self](auto&& catalog){ catalog->Initialize(self,catalog); });
	}
}
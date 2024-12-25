#pragma once
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/DBSchema.h>
#include "ForeignKey.h"
#include "Procedure.h"


namespace Jde::DB{
	struct AppSchema; struct Catalog; struct ForeignKey; struct IServerMeta; struct Procedure; struct Syntax; struct View;

	//Cluster > Catalog > Schema > Table > Columns & Rows
	struct SchemaDdl : DBSchema, std::enable_shared_from_this<SchemaDdl>{
		SchemaDdl( sv name, sv tablePrefix, const IServerMeta& loader )ε;
		Ω Sync( const AppSchema& config )ε->void;
		Ω Create( const DBSchema& config )ε->void;

		α Tables()ι->flat_map<string,sp<Table>>&{ return Meta()->Tables; }
		α Views()ι->flat_map<string,sp<View>>&{ return Meta()->Views; }

		flat_map<string,Procedure> Procs;
		flat_map<string,ForeignKey> FKs;//fk name, fk
	private:
		α SyncData( const AppSchema& config, const jobject& jconfig )ε->void;
		α SyncFKs( const AppSchema& config )ε->void;
		α SyncScripts(const AppSchema& config, const jobject& jconfig )ε->void;
		α SyncTables( const AppSchema& config )ε->void;

		α Meta()ι->sp<AppSchema>{ return AppSchemas.find("")->second; }
#ifndef PROD
	public:
		Ω Drop( const AppSchema& config )ε->void;
#endif
	};
}
#pragma once
#include <jde/db/meta/Schema.h>
#include "ForeignKey.h"
#include "Procedure.h"


namespace Jde::DB{
	struct Catalog; struct ForeignKey; struct IServerMeta; struct Procedure; struct Syntax; struct View;

	//Cluster > Catalog > Schema > Table > Columns & Rows
	struct SchemaDdl : Schema, std::enable_shared_from_this<SchemaDdl>{
		SchemaDdl( sv name, const IServerMeta& loader )ε;
		Ω Sync( const Schema& config )ε->void;
		Ω Create( const Schema& config )ε->void;

		flat_map<string,Procedure> Procs;
		flat_map<string,ForeignKey> FKs;//fk name, fk
	private:
		α SyncData( const Schema& config, const jobject& jconfig )ε->void;
		α SyncFKs( const Schema& config )ε->void;
		α SyncScripts(const Schema& config, const jobject& jconfig )ε->void;
		α SyncTables( const Schema& config )ε->void;

#ifndef PROD
	public:
		Ω Drop( const Schema& config )ε->void;
#endif
	};
}
#pragma once
#include <jde/db/metadata/Schema.h>
#include "Index.h"

namespace Jde::DB{
	struct Catalog; struct ForeignKey; struct Procedure; struct Syntax; struct View;
	//Cluster > Catalog > Schema > Table > Columns & Rows
	struct SchemaDdl : Schema, std::enable_shared_from_this<SchemaDdl>{

		α Sync( const Schema& config, const jobject& jconfig, const fs::path& relativeScriptPath )ε->void;

		flat_map<string,Procedure> Procedures;
		flat_map<string,ForeignKey> ForeignKeys;//fk name, fk
	private:
		α SyncData( const Schema& config, const jobject& jconfig )ε->void;
		α SyncForeignKeys( const Schema& config )ε->void;
		α SyncScripts(const Schema& config, const jobject& jconfig, const fs::path& relativeScriptPath )ε->void;
		α SyncTables( const Schema& config )ε->void;
	};
}
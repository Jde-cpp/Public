#include "SchemaDdl.h"
#include <jde/fwk/io/file.h>
#include "TableDdl.h"
#include <jde/db/IDataSource.h>
#include <jde/db/names.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/generators/Syntax.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Catalog.h>
#include <jde/db/meta/Cluster.h>
#include <jde/db/meta/Column.h>
#include "../IServerMeta.h"
#include <jde/ql/IQL.h>
#include "Index.h"
#include "Procedure.h"


#define let const auto
namespace Jde::DB{
	constexpr ELogTags _tags{ ELogTags::Sql };

	SchemaDdl::SchemaDdl( str name, sv tablePrefix, const IServerMeta& loader, sp<QL::IQL> ql )ε:
		DBSchema{ name, loader.LoadTables(name, tablePrefix), tablePrefix },
		Procs{ loader.LoadProcs(name) },
		FKs{ loader.LoadForeignKeys(name) },
		_ql{ql}
	{}

	α abbrevName( sv schemaName )ι->string;
	α GetData( const Table& table, const jobject& j )ε->vector<flat_map<string,Value>>;
	α Exists( const DBSchema& config )ι->bool;
	α GetFlagsData( const jobject& j )ε->flat_map<uint,Value>;
	α UniqueIndexName( const Index& index, bool uniqueName, const vector<Index>& indexes )ε->string;

	α ConfigurationJson( const AppSchema& config )ε->const jobject{
		let appSchema = Settings::AsObject( config.ConfigPath() );
		auto appMeta = Json::ReadJsonNet( Json::AsSV(appSchema, "meta") );
		if( let prefix = Json::FindString(appSchema, "prefix"); prefix )
			appMeta["prefix"] = *prefix;
		return appMeta;
	}
//todo doc relativeScriptPath
//todo add db version table.
	α SchemaDdl::Sync( const AppSchema& config, sp<QL::IQL> ql )ε->void{
		if( !Exists(*config.DBSchema) ){
			Create( *config.DBSchema );
			config.ResetDS();
		}
		let& initConfig = ConfigurationJson( config );//has data, scripts.
		string prefix = config.Prefix;
		if( uint index = prefix.find('.'); index!=string::npos )
			prefix = index<prefix.size()-2 ? string{} : prefix.substr( index+1 );

		auto catalog = ms<DB::Catalog>( config.DS() );
		auto db = ms<SchemaDdl>( config.DBSchema->Name, prefix, config.DS()->ServerMeta(), ql ); //dbSchema
		db->Initialize( catalog, db );
		catalog = nullptr;

		db->SyncTables( config );
		db->SyncScripts( config, initConfig );
		db->SyncData( config, Json::AsObject(initConfig, "tables") );
		db->SyncFKs( config );
		wp<AppSchema> appSchema = db->AppSchemas.begin()->second;
		wp<SchemaDdl> db2 = db;
		db = nullptr;
	}

	α SchemaDdl::SyncFKs( const AppSchema& config )ε->void{
		for( auto& [tableName, table] : config.Tables ){
			for( auto& column : table->Columns ){
				if( !column->PKTable )
					continue;
				if( find_if(FKs, [&,t=config.ObjectPrefix()+table->Name](let& fk){
					return fk.second.Table==t && fk.second.Columns==vector<string>{column->Name};
				})!=FKs.end() ){
					continue;
				}
				let pkTable = column->PKTable;
				if( column->IsFlags() )
					continue;
				auto getName = [&, &t=tableName](auto i)->string {//&t for clang
					return Ƒ( "{}_{}{}_fk", abbrevName(t), abbrevName(pkTable->Name), i==0 ? "" : std::to_string(i) );
				};
				uint i{};
				auto name = getName( i++ );
				for( ; FKs.find(name)!=FKs.end(); name = getName(i++) );

				auto createStatement = ForeignKey::Create( name, column->Name, *pkTable, *table );
				config.DS()->ExecuteSync( {move(createStatement)} );
				FKs.emplace( name, ForeignKey{name, table->Name, {column->Name}, pkTable->Name} );
				INFO( "Created fk '{}'.", name );
			}
		}
	}
	Ω forEachDir( sv jpath, sv extension, vector<string> prefixes, function<void(const fs::path& file)> process )ε{
		let dirs = Settings::FindStringArray( jpath );
		for( let& scriptDir : dirs ){
			const fs::path scriptRoot{ scriptDir };
			DBGT( ELogTags::App, "Processing '{}'.  Prefixes: [{}], extension: {}", scriptRoot.string(), Str::Join(prefixes, ", "), extension );
			THROW_IF( !fs::exists(scriptRoot) || !fs::is_directory(scriptRoot), "Script path '{}' does not exist.", scriptRoot.string() );
			flat_map<string,fs::path> files;  //abc order
			for( let& entry : fs::directory_iterator(scriptRoot) ){
				if( let& path = entry.path();
					!entry.is_directory()
						&& path.extension().string().starts_with(extension)
						&& (prefixes.empty() || find_if( prefixes, [&](let& x){ return path.filename().string().starts_with(x);})!=prefixes.end()) ){
					files.emplace( path.filename().string(), path );
				}
			}
			for( let& [fileName, file] : files )
				process( file );
		}
	}

	α SchemaDdl::SyncData( const AppSchema& config, const jobject& initConfig )ε->void{
		vector<string> prefixes{ config.Name };
		if( let& configPrefixes = Json::FindArray(initConfig, "dataPrefixes"); configPrefixes ){
			auto additional = Json::FromArray<string>( *configPrefixes );
			move( additional.begin(), additional.end(), std::back_inserter(prefixes) );
		}
		forEachDir( "/dbServers/dataPaths", ".mutation", prefixes, [this](const fs::path& file){
			let text = IO::Load( file );
			INFO( "Mutation: '{}'", file.string() );
			_ql->Upsert( text, {UserPK::System} );
		});
	}

	α SchemaDdl::SyncScripts( const AppSchema& config, const jobject& initConfig )ε->void{
		//let& syntax = config.DS()->Syntax();
		auto prefix = Json::FindString( initConfig, "prefix" ).value_or( "" );
//		if( config.DBSchema->Name.size() && config.DBSchema->Name[0]!='_' )
//			prefix = config.DBSchema->Name+ "." + prefix;
		let stdPrefix = config.Name+"_"; //access_
		forEachDir( "/dbServers/scriptPaths", ".sql", {stdPrefix}, [&](const fs::path& scriptFile){
			let fileName = scriptFile.filename(); //[access_]user_insert.sql
			let stem = fileName.stem().string();
			let procViewName = stem.substr( stdPrefix.size() );
			let dbName = prefix+procViewName;
			if( Procs.find(dbName)!=Procs.end() || Tables().find(procViewName)!=Tables().end() )
				return;
			let text = IO::Load( scriptFile );
			TRACE( "Executing '{}'", scriptFile.string() );
			let queries = Str::Split<sv,Str::iv>( text, "\ngo"_iv );
			for( let& text : queries ){
				string newName = Ƒ("[{}]", config.DBSchema->Name);
				let query = Str::Replace(
					Str::Replace(text, "[dbo]"sv, Ƒ("[{}]", config.DBSchema->Name) ),
					stdPrefix, prefix );

				std::ostringstream os;
				for( uint i=0; i<query.size(); ++i ){
					if( query[i]=='#' )
						for( ; i<query.size() && query[i]!='\n'; ++i );
					if( i<query.size() )
						os.put( query[i] );
				}
				config.DS()->ExecuteSync( {os.str()} );
			}
			INFO( "Finished '{}'", scriptFile.string() );
		});
	}

	α SchemaDdl::SyncTables( const AppSchema& config )ε->void{
		const DB::Syntax& syntax = config.DS()->Syntax();
		IDataSource& ds = *DS();
		let& schemaName = config.DBSchema->Name;
		for( let& [tableName, table] : config.Tables ){
			sp<TableDdl> dbTable;
			if( let kv=Tables().find(config.ObjectPrefix()+tableName); kv!=Tables().end() ){
				dbTable = std::dynamic_pointer_cast<TableDdl>( kv->second );
				for( auto& column : table->Columns ){
					auto pDBColumn = dbTable->FindColumn( column->Name ); if( !pDBColumn ){ CRITICAL("Could not find db column {}.{}", tableName, column->Name); continue; }
					pDBColumn->Insertable = column->Insertable;
					if( pDBColumn->Default && pDBColumn->Default->is_string() && pDBColumn->Default->get_string()!="$now" )
						ds.TryExecuteSync( {syntax.AddDefault(table->DBName, column->Name, *pDBColumn->Default)} );
				}
			}
			else{
				dbTable = ms<TableDdl>( *table );
				ds.ExecuteSync( {dbTable->CreateStatement()} );
				INFO( "Created table '{}'.", table->DBName );
				dbTable = ds.ServerMeta().LoadTable( schemaName, config.ObjectPrefix()+table->Name );
				dbTable->Initialize( FindAppSchema( "" ), dbTable );
				Tables().emplace( dbTable->Name, dbTable );
			}

			auto& dbIndexes = dbTable->Indexes;
			for( let& index : Index::GetConfig(*table) ){
				if( find_if(dbIndexes, [&](let& db){ return db.TableName==config.ObjectPrefix()+tableName && db.Columns==index.Columns;} )!=dbIndexes.end() )
					continue;
				let name = UniqueIndexName( index, syntax.UniqueIndexNames(), dbIndexes );
				ds.ExecuteSync( {index.Create(name, schemaName+'.'+dbTable->DBName, syntax)} );
				dbIndexes.push_back( Index{name, tableName, index} );
				INFO( "Created index '{}.{}'.", table->DBName, name );
			}
			if( auto procName = table->HasCustomInsertProc ? "" : table->InsertProcName(); procName.size() ){
				if( let index = procName.find_first_of('.'); index<procName.size()-1 )
					procName = procName.substr( index+1 );
				if( Procs.find(procName)!=Procs.end() )
						continue;

				ds.ExecuteSync( {dbTable->InsertProcCreateStatement(*table)} );
				Procs.emplace( procName, Procedure{procName} );
				INFO( "Created proc '{}'.", table->InsertProcName() );
			}
		}
		for( let& [name, view] : config.Views ){
			Views().emplace( name, view );
			//p->Initialize( Meta(), p );
			//placeholder
		}
		for( let& [_, table] : Tables() ){
			table->Initialize( Meta(), table );
		}
	}

	α SchemaDdl::Create( const DBSchema& config )ε->void{
		auto ds = config.DS()->Syntax().CanSetDefaultSchema()
			? config.DS()->AtSchema( config.DS()->Syntax().SysSchema() )
			: config.DS();
		ds->ExecuteSync( {Ƒ("CREATE SCHEMA {}", config.Name)} );
	}
#ifndef PROD
	α DropObjects( const AppSchema& config )ε->void{
		auto& ds = *config.DS();

		for( auto& [name, fk] : config.DS()->ServerMeta().LoadForeignKeys(config.Name) ){
			if( find_if(config.Tables, [&fk](let& t){return t.second->DBName==fk.Table;})!=config.Tables.end() )
				ds.ExecuteSync( {Ƒ("ALTER TABLE {} DROP CONSTRAINT {}", fk.Table, name)} );
		}

		for( let& [tableName, table] : config.Tables ){
			if( table->InsertProcName().size() )
				ds.ExecuteSync( {Ƒ("DROP PROCEDURE IF EXISTS {}", table->InsertProcName())} );
			if( table->PurgeProcName.size() )
				ds.ExecuteSync( {Ƒ("DROP PROCEDURE IF EXISTS {}", table->PurgeProcName)} );
			ds.ExecuteSync( {Ƒ("DROP TABLE IF EXISTS {}", table->DBName)} );
		}
		let& initConfig = ConfigurationJson( config );
		if( auto script = Json::FindString(initConfig, "meta"); script ){
			auto name = fs::path{ *script }.stem().string();
			let type = name.ends_with("_ql") ? "VIEW" : "PROCEDURE";
			ds.ExecuteSync( {Ƒ("DROP {} IF EXISTS {}", type, name)} );
		}
	}

	α SchemaDdl::Drop( const AppSchema& config )ε->void{
		if( Exists(*config.DBSchema) ){
			// if catalogs, would have dropped catalog
			//if( !config.DS()->Syntax().SchemaDropsObjects() )
			//	DropObjects( config );
			let catalogName = config.DS()->CatalogName();
			config.DS()->ExecuteSync( {Ƒ("DROP SCHEMA {}", config.DBSchema->Name)} );
		}
	}
#endif

	α abbrevName( sv schemaName )ι->string{
		auto fnctn = []( let& word )->string {
			std::ostringstream os;
			for( let ch : word ){
				if( (ch!='a' && ch!='e' && ch!='i' && ch!='o' && ch!='u') || os.tellp() == std::streampos(0) )
					os << ch;
			}
			return word.size()>2 && os.str().size()<word.size()-1 ? os.str() : string{ word };
		};
		let singular = Names::ToSingular( schemaName );
		let splits = Str::Split( singular, '_' );
		std::ostringstream name;
		for( uint i=1; i<splits.size(); ++i ){
			if( i>1 )
				name << '_';
			name << fnctn( splits[i] );
		}
		return name.str();
	}

	α Exists( const DBSchema& config )ι->bool{
		try{
			return config.DS()->ScalerSyncOpt<string>( {"SELECT SCHEMA_NAME FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = ?", {Value{config.Name}}} ).has_value();
		}
		catch( IException& e ){
			e.SetLevel( ELogLevel::Debug );
			e.Log();
			return false;//connected to schema which doesn't exist.
		}
	}

	α GetData( const Table& table, const jobject& j )ε->vector<flat_map<string,Value>>{
		vector<flat_map<string,Value>> data;
		auto jdata = Json::FindArray( j, "data" );
		if( !jdata )
			return data;

		uint id = 0;
		for( let& jrow : *jdata ){
			flat_map<string,Value> row;
			if( jrow.is_string() ){
				row[table.GetPK()->Name] = ++id;
				row["name"] = Value{ string{jrow.get_string()} };
			}
			else if( jrow.is_object() ){
				for( let& [name, value] : jrow.get_object() ){
					if( name=="comment" )
						continue;
					let c = table.GetColumnPtr( name=="id" ? table.GetPK()->Name : Names::FromJson(name) );
					row[c->Name] = Value{ c->Type, value };
				}
			}
			else
				THROW( "Invalid data '{}' expecting string or object.", serialize(jrow) );
			data.push_back( row );
		}
		return data;
	}

	α GetFlagsData( const jobject& j )ε->flat_map<uint,Value>{
		flat_map<uint,Value> flagsData;
		if( auto kv = j.find("flagsData"); kv!=j.end() ){
			uint i=0;
			for( let& col : kv->value().as_array() ){
				flagsData.emplace( i==0 ? 0 : 1 << (i-1), Value{string{col.as_string()}} );
				++i;
			}
		}
		return flagsData;
	}

	α UniqueIndexName( const DB::Index& index, bool uniqueName, const vector<Index>& indexes )ε->string{
		auto indexName=index.Name;
		bool checkOnlyTable = !index.PrimaryKey && !uniqueName;
		for( uint i=2; ; indexName = Ƒ( "{}{}", index.Name, i++ ) ){
			if( find_if(indexes, [&](let& x){ return ToIV(x.Name)==ToIV(indexName) && (!checkOnlyTable || index.TableName==x.TableName);})==indexes.end() )
				break;
		}
		return indexName;
	}
}
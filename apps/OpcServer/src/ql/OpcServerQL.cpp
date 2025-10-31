#include "OpcServerQL.h"
#include <jde/fwk/io/file.h>
#include <jde/db/generators/InsertClause.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/ql/ql.h>
#include <jde/ql/IQL.h>
#include <jde/ql/QLAwait.h>
#include "../awaits/ServerConfigAwait.h"
#include "../StartupAwait.h"
#define let const auto

namespace Jde::Opc::Server{
	α UpsertAwait::await_ready()ι->bool{
		if( let configRoot = Settings::FindPath("/opcServer/mutationsDir"); configRoot && fs::exists(*configRoot) ){
			for( let& entry : fs::directory_iterator(*configRoot) ){
				let& path = entry.path();
				if( !entry.is_directory() && path.extension().string().starts_with(".mutation") )
					_files.push_back( path );
			}
		}
		return _files.empty();
	}
	α UpsertAwait::Execute()->TAwait<jvalue>::Task{
		try{
			jarray y;
			for( let& file : _files ){
				INFOT( ELogTags::Startup, "Mutation: '{}'", file.string() );
				let text = IO::Load( file );
				auto requests = QL::Parse( move(text), {}, Schemas() ); THROW_IF( !requests.IsMutation(), "Query is not a mutation" );
				for( auto&& m : requests.Mutations() ){
					m.Args["$silent"] = true;
					try{
						y.push_back( co_await QL::QLAwait<jvalue>{move(m), {UserPK::System}} );
					}
					catch( IException& )//assume already exists
					{}
				}
			}
			DBGT( ELogTags::App, "Upsert: {}", serialize(y) );
			Resume();
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}
#include "Resource.h"
#include <jde/db/Value.h>
#include <jde/db/IDataSource.h>
#include <jde/db/await/RowAwait.h>
#include <jde/db/meta/Schema.h>
#include <jde/db/meta/Table.h>
#include <jde/db/generators/FromClause.h>
#include <jde/db/generators/WhereClause.h>

#define let const auto
namespace Jde::Access{
	constexpr ELogTags _tags{ ELogTags::Access };

	ResourceLoadAwait::ResourceLoadAwait( sp<DB::Schema> schema, vector<AppPK> appPKs )ι:
		AppPKs{ appPKs },
		_schema{ schema }
	{}

	α LoadResources( sp<DB::Schema> schema, ResourceLoadAwait& await )ι->DB::RowAwait::Task{
		flat_map<ResourcePK,Resource> resources;
		try{
			sp<DB::Table> resourceTable = schema->GetTablePtr( "resources" );
			let ds = resourceTable->Schema->DS();
			const DB::WhereClause where{ resourceTable->GetColumnPtr("app_id"), DB::ToValue(await.AppPKs) };
			auto statement = DB::SelectSql( {"resource_id","app_id","target","filter"}, resourceTable, where );
			auto rows = co_await ds->SelectCo( move(statement) );
			for( let& row : rows ){
				let pk = row->GetUInt16(0);
				resources.emplace( pk, Resource{pk, row->GetUInt16(1), row->GetString(2), row->GetString(3)} );
			}
			sp<DB::Table> resourceRightsTable = schema->GetTablePtr( "resource_rights" );
			vector<string> columns{"permission_id","resource_id", "allowed", "denied"};
			DB::FromClause from{ {resourceTable, resourceRightsTable} };
			statement = DB::SelectSql( columns, from, where );
			rows = co_await ds->SelectCo( move(statement) );
			for( let& row : rows ){
				let pk = row->GetUInt16(0);
				let resourceId = row->GetUInt16(1);
				if( auto p = resources.find(resourceId); p!=resources.end() )
					p->second.Permissions.emplace( pk, Permission{pk, (ERights)row->GetUInt16(2), (ERights)row->GetUInt16(3)} );
				else
					Warning{ _tags, "[{}]Resource not found for permission: {}.", resourceId, pk };
			}
			await.Resume( move(resources) );
		}
		catch( IException& e ){
			await.ResumeExp( move(e) );
		}
	}

	α ResourceLoadAwait::Suspend()ι->void{
		LoadResources( _schema, *this );
	}
}
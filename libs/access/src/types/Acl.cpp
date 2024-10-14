#include "Acl.h"
#include <jde/db/IDataSource.h>

#define let const auto
namespace Jde::Access{

	α LoadAcl( sp<DB::Schema> schema, AclLoadAwait& await )ι->DB::RowAwait::Task{
		try{
			sp<DB::Table> aclTable = schema->GetTablePtr( "acl" );
			let ds = aclTable->Schema->DS();
			auto statement = DB::SelectSKsSql( aclTable );
			let rows = co_await ds->SelectCo( move(statement) );
			flat_multimap<IdentityPK,PermissionPK> acl;
			for( let& row : rows )
				acl.emplace( row->GetUInt16(0), row->GetUInt16(1) );
			await.Resume( move(acl) );
		}
		catch( IException& e ){
			await.ResumeExp( move(e) );
		}
	}

	void AclLoadAwait::Suspend()ι{
		LoadAcl( _schema, *this );
	}
}
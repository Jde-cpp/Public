//ql-review3 #58: introspectFields walks the *jsonnet-declared* columns, and addColumn resolves against them too, so a column
//the meta declares but the dialect's view does not select is advertised to clients and then emitted into SQL that the database
//refuses - `providersQL.providerTypeId` (sqlServer's view selects it, sqlite's and mysql's did not) and `groupMembers.providerId`
//(no dialect's did).  Nothing in ql could catch that: the meta is the only thing it can see.
//This is the parity check the write-up asks for - every declared column of every view, selected from that view - so the next
//drift is a failing test rather than a client-visible "no such column".  It runs against sqlite; the other two dialects have no
//suite here, which is exactly how the sqlServer/sqlite divergence survived.
#include "gtest/gtest.h"
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Sql.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include "globals.h"

#define let const auto

namespace Jde::Access::Tests{
	//`where 1=0` - the statement has to *prepare*, which is what names a missing column;  no rows are wanted.
	Ω selectDeclaredColumns( const DB::View& view )ε->void{
		string columns;
		for( let& c : view.Columns )
			columns += (columns.empty() ? "" : ", ")+c->Name;
		ASSERT_FALSE( columns.empty() ) << view.Name;
		DS().Select( DB::Sql{Ƒ("select {} from {} where 1=0", columns, view.SqlName()), {}}, [](DB::Row&&){} );
	}

	TEST( ViewParityTests, EveryDeclaredViewColumnExistsInTheView ){
		let schemas = Schemas();
		ASSERT_FALSE( schemas.empty() );
		uint checked{};
		for( let& schema : schemas ){
			for( let& [name, view] : schema->Views ){
				try{
					selectDeclaredColumns( *view );
					++checked;
				}
				catch( const Exception& e ){
					ADD_FAILURE() << name << ": " << e.what();//the meta declares a column this dialect's view does not select.
				}
			}
		}
		EXPECT_GT( checked, 0u ) << "no views were checked - the walk found nothing to assert";
	}

	//and the tables, on the same rule:  a declared column that the DDL did not create fails identically, and the walk is free.
	TEST( ViewParityTests, EveryDeclaredTableColumnExistsInTheTable ){
		let schemas = Schemas();
		for( let& schema : schemas ){
			for( let& [name, table] : schema->Tables ){
				try{
					selectDeclaredColumns( *table );
				}
				catch( const Exception& e ){
					ADD_FAILURE() << name << ": " << e.what();
				}
			}
		}
	}
}

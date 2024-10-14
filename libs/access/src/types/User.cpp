#include "User.h"
#include <jde/framework.h>
#include <jde/framework/coroutine/Await.h>
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Statement.h>
#include <jde/db/meta/Schema.h>
#include <jde/db/meta/Table.h>

#define let const auto
namespace Jde::Access{
	using namespace Jde::Coroutine;

	UserLoadAwait::UserLoadAwait( sp<DB::Schema> schema )ι:
		_schema{ schema }
	{}

	α UserLoadAwait::Execute()ι->Coroutine::Task{
		let userTable{ _schema->GetTablePtr("users") };
		let ds = userTable->Schema->DS();
		try{
			auto pks = ( co_await ds->SelectSet<UserPK>(DB::SelectSKsSql(userTable)) ).UP<flat_set<UserPK>>();
			[this]( auto ds, auto pks )ι->TAwait<flat_multimap<GroupPK,UserPK>>::Task {
				try{
					let userGroups = co_await *ds->template SelectMultiMap<GroupPK,UserPK>( DB::SelectSKsSql(_schema->GetTablePtr("groups")) );
					concurrent_flat_map<UserPK,User> users;
					for( auto&& [groupPk,userPk] : userGroups )
						users.visit( userPk, [&](auto& kv){ kv.second.Groups.emplace(groupPk); } );
					Resume( std::move(users) );
				}
				catch( IException& e ){
					ResumeExp( move(e) );
				}
			}(ds, std::move(pks));
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}
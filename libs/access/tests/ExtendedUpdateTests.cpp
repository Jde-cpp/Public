//ql-review3 #45: an extended table's own row is keyed by whatever the parent's where clause was keyed by
//(UpdateAwait::CreateUpdate `rowKey = update.Where.Params()[0]`), and on the name/target branches that is the literal - so
//`updateUser( name:"bob", loginName:… )` emitted `update access_users set … where access_users.identity_id='bob'`, which
//matches nothing, and reported the *identities* row count as success.  By id it was always right, so the shape is refused.
//Second half, db-locus: UpdateClause::Move looked for `updated` through Table::FindColumn, which resolves via Extends, so the
//users statement carried identities' `updated` and every update of a users-owned column failed with "no such column: updated"
//- after the identities half had already committed, untransacted.  Fixed together: without it the by-id path below still fails.
#include "gtest/gtest.h"
#include "globals.h"

#define let const auto

namespace Jde::Access::Tests{
	//the write-up's own shape.  Nothing is written: the guard is at the top of CreateUpdate, before the parent's clause is
	//built, so this is not a half-applied mutation the way the old failure was.
	TEST( ExtendedUpdateTests, UpdatingAnExtendedTableByNameIsRefused ){
		let root = GetRoot();
		const string target{ "review45-byName" };
		let user = UserPK{ GetId(GetUser(target, root)) };
		let before = Select( "user", user.Value, root, "id name loginName", true );

		try{
			QL().QuerySync<jvalue>( "mutation updateUser( name:\""+string{Json::AsSV(before,"name")}+"\", loginName:\"review45-refused\" )", {}, root );
			ADD_FAILURE() << "the by-name update was accepted";
		}
		catch( const Exception& e ){
			EXPECT_NE( string{e.what()}.find("users"), string::npos ) << e.what();      //which table,
			EXPECT_NE( string{e.what()}.find("identities"), string::npos ) << e.what(); //and what it extends.
		}
		let after = Select( "user", user.Value, root, "id name loginName", true );
		EXPECT_EQ( serialize(after), serialize(before) ) << "the refusal still wrote something";
		PurgeUser( user, root );
	}

	//by id, which is the spelling that was always correct - and the one that could not work until UpdateClause::Move stopped
	//borrowing the parent's `updated`.  The read-back is the point: the extension row really changed.
	TEST( ExtendedUpdateTests, UpdatingAnExtendedTableByIdChangesTheExtensionRow ){
		let root = GetRoot();
		const string target{ "review45-byId" };
		let user = UserPK{ GetId(GetUser(target, root)) };

		QL().QuerySync<jvalue>( "mutation updateUser( id:"+std::to_string(user.Value)+", loginName:\"review45-login\" )", {}, root );
		EXPECT_EQ( Json::AsSV(Select("user", user.Value, root, "id loginName", true), "loginName"), "review45-login" );

		//and a column of the parent, through the same statement pair - the identities half was never the broken one.
		QL().QuerySync<jvalue>( "mutation updateUser( id:"+std::to_string(user.Value)+", name:\"review45-renamed\" )", {}, root );
		let after = Select( "user", user.Value, root, "id name loginName", true );
		EXPECT_EQ( Json::AsSV(after, "name"), "review45-renamed" );
		EXPECT_EQ( Json::AsSV(after, "loginName"), "review45-login" ); //unchanged by the second statement.
		PurgeUser( user, root );
	}

	//the control, and the reason the guard is where it is rather than at the top of CreateUpdate:  `groups` extends identities
	//too, but `description` is a *parent* column, so no extension statement is emitted and the natural key is still correct.
	//Refusing this as well would have cost a working shape to fix a broken one.
	TEST( ExtendedUpdateTests, AnExtendedTableIsStillUpdateableByNameWhenOnlyParentColumnsChange ){
		let root = GetRoot();
		const string target{ "review45-plain" };
		let group = GroupPK{ GetId(GetGroup(target, root)) };
		let name = string{ Json::AsSV(Select("group", group.Value, root, "id name", true), "name") };

		QL().QuerySync<jvalue>( "mutation updateGroup( name:\""+name+"\", description:\"review45-desc\" )", {}, root );
		EXPECT_EQ( Json::AsSV(Select("group", group.Value, root, "id description", true), "description"), "review45-desc" );
		PurgeGroup( group, root );
	}
}

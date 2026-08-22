#include "gtest/gtest.h"
#include <jde/fwk/io/json.h>
#include <jde/access/Authorize.h>

#define let const auto
namespace Jde::Access::Tests{
	using enum ERights;
	//in-memory unit tests - no db/ql. exercises the recalculation logic directly.
	struct TestAuthorize final : Authorize{
		TestAuthorize()ι:Authorize{"AuthorizeTests"}{}
		using Authorize::Acl;
		using Authorize::AddAcl;
		using Authorize::AddToGroup;
		using Authorize::CreateResource;
		using Authorize::CreateUser;
		using Authorize::DeleteGroup;
		using Authorize::Groups;
		using Authorize::Permissions;
		using Authorize::PurgeGroup;
		using Authorize::PurgeRole;
		using Authorize::PurgeUser;
		using Authorize::RemoveAcl;
		using Authorize::RemoveFromGroup;
		using Authorize::RestoreGroup;
		using Authorize::Roles;
		using Authorize::UpdatePermission;
	};
	constexpr ResourcePK _resourcePK{ 1 };
	const UserPK _user{ 100 };
	const string _schema{ "unitTest" };
	const string _target{ "widgets" };

	Ω createAuthorizer()ε->sp<TestAuthorize>{
		auto auth = ms<TestAuthorize>();
		auth->CreateResource( Resource{_resourcePK, jobject{{"schemaName",_schema},{"target",_target}}} );
		auth->AddResource( _resourcePK, _schema, _target, {} );
		auth->CreateUser( _user );
		return auth;
	}
	Ω rights( TestAuthorize& auth )ι->ERights{ return auth.Rights(_schema, _target, _user); }

	TEST( AuthorizeTests, RemoveAclRevokes ){
		auto auth = createAuthorizer();
		auth->AddAcl( _user.Value, PermissionPK{10}, Read, None, _resourcePK );
		ASSERT_EQ( rights(*auth), Read );
		auth->RemoveAcl( _user.Value, PermissionRole{std::in_place_index<0>, PermissionPK{10}} );
		ASSERT_EQ( rights(*auth), None );
	}

	TEST( AuthorizeTests, UpdatePermissionDeniedOnly ){
		auto auth = createAuthorizer();
		auth->AddAcl( _user.Value, PermissionPK{10}, Read, None, _resourcePK );
		auth->UpdatePermission( PermissionPK{10}, {}, Delete );//denied-only update must not wipe allowed.
		ASSERT_EQ( rights(*auth), Read );
	}

	TEST( AuthorizeTests, UpdatePermissionSiblings ){
		auto auth = createAuthorizer();
		auth->AddAcl( _user.Value, PermissionPK{10}, Read, None, _resourcePK );
		auth->AddAcl( _user.Value, PermissionPK{11}, Create, None, _resourcePK );
		ASSERT_EQ( rights(*auth), Read | Create );
		auth->UpdatePermission( PermissionPK{10}, Update, {} );//sibling 11 must keep its own rights.
		ASSERT_EQ( rights(*auth), Update | Create );
	}

	TEST( AuthorizeTests, NestedGroupRevocation ){
		auto auth = createAuthorizer();
		const GroupPK parent{ 200 }, child{ 201 };
		auth->AddToGroup( child, {_user.Value} );
		auth->AddToGroup( parent, {child.Value} );
		auth->AddAcl( parent.Value, PermissionPK{10}, Read, None, _resourcePK );
		ASSERT_EQ( rights(*auth), Read );
		auth->RemoveFromGroup( parent, {child.Value} );//removing nested group must clear its users' rights.
		ASSERT_EQ( rights(*auth), None );
	}

	TEST( AuthorizeTests, DeleteRestoreGroup ){
		auto auth = createAuthorizer();
		const GroupPK group{ 240 };
		auth->AddToGroup( group, {_user.Value} );
		auth->AddAcl( group.Value, PermissionPK{10}, Read, None, _resourcePK );
		ASSERT_EQ( rights(*auth), Read );
		auth->DeleteGroup( group );//soft delete - members lose the group's grants...
		ASSERT_EQ( rights(*auth), None );
		auth->RestoreGroup( group );//...and the row survives, so restore puts them back.
		ASSERT_EQ( rights(*auth), Read );
	}

	TEST( AuthorizeTests, DeleteRestoreNestedGroup ){
		auto auth = createAuthorizer();
		const GroupPK parent{ 250 }, child{ 251 };
		auth->AddToGroup( child, {_user.Value} );
		auth->AddToGroup( parent, {child.Value} );
		auth->AddAcl( parent.Value, PermissionPK{10}, Read, None, _resourcePK );
		ASSERT_EQ( rights(*auth), Read );
		auth->DeleteGroup( parent );
		ASSERT_EQ( rights(*auth), None );
		auth->RestoreGroup( parent );
		ASSERT_EQ( rights(*auth), Read );
	}

	TEST( AuthorizeTests, DeletedGroupDoesNotRegrant ){
		auto auth = createAuthorizer();
		const GroupPK group{ 260 };
		auth->AddToGroup( group, {_user.Value} );
		auth->AddAcl( group.Value, PermissionPK{10}, Read, None, _resourcePK );
		auth->DeleteGroup( group );
		ASSERT_EQ( rights(*auth), None );
		auth->AddToGroup( group, {_user.Value} );//a later recalc must not re-apply a deleted group's acl.
		ASSERT_EQ( rights(*auth), None );
	}

	TEST( AuthorizeTests, DeletedGroupPurgeableAndRestorable ){
		auto auth = createAuthorizer();
		const GroupPK deleted{ 270 }, purged{ 271 };
		auth->AddToGroup( deleted, {_user.Value} );
		auth->AddToGroup( purged, {_user.Value} );
		auth->AddAcl( deleted.Value, PermissionPK{10}, Read, None, _resourcePK );
		auth->AddAcl( purged.Value, PermissionPK{11}, Create, None, _resourcePK );
		ASSERT_EQ( rights(*auth), Read | Create );
		auth->DeleteGroup( purged );
		auth->PurgeGroup( purged );//delete-then-purge: the deleted branch of PurgeGroup, reachable now.
		ASSERT_FALSE( auth->Groups.contains(purged) );
		ASSERT_EQ( rights(*auth), Read );//the other group's grant is untouched.
	}

	TEST( AuthorizeTests, PurgeGroupRevokes ){
		auto auth = createAuthorizer();
		const GroupPK group{ 210 };
		auth->AddToGroup( group, {_user.Value} );
		auth->AddAcl( group.Value, PermissionPK{10}, Read, None, _resourcePK );
		ASSERT_EQ( rights(*auth), Read );
		auth->PurgeGroup( group );//purging an active group must clear its members' rights.
		ASSERT_EQ( rights(*auth), None );
	}

	TEST( AuthorizeTests, PurgeNestedGroupRevokes ){
		auto auth = createAuthorizer();
		const GroupPK parent{ 220 }, child{ 221 };
		auth->AddToGroup( child, {_user.Value} );
		auth->AddToGroup( parent, {child.Value} );
		auth->AddAcl( parent.Value, PermissionPK{10}, Read, None, _resourcePK );
		ASSERT_EQ( rights(*auth), Read );
		auth->PurgeGroup( parent );//the grant is on the parent - the nested member must lose it too.
		ASSERT_EQ( rights(*auth), None );
	}

	TEST( AuthorizeTests, PurgeDeletedGroupKeepsDirectGrant ){
		auto auth = createAuthorizer();
		const GroupPK group{ 230 };
		auth->AddToGroup( group, {_user.Value} );
		auth->AddAcl( _user.Value, PermissionPK{11}, Create, None, _resourcePK );
		auth->Groups.find( group )->second.IsDeleted = true;//soft-deleted row - members already recalculated.
		auth->PurgeGroup( group );//the deleted branch just erases, unrelated grants must survive.
		ASSERT_EQ( rights(*auth), Create );
		ASSERT_FALSE( auth->Groups.contains(group) );
	}

	TEST( AuthorizeTests, AclUpsertLowersRights ){
		auto auth = createAuthorizer();
		auth->AddAcl( _user.Value, PermissionPK{10}, Read | Update, None, _resourcePK );
		ASSERT_EQ( rights(*auth), Read | Update );
		auth->AddAcl( _user.Value, PermissionPK{10}, Read, None, _resourcePK );//re-grant is an upsert on the same pk - it must lower.
		ASSERT_EQ( rights(*auth), Read );
		ASSERT_EQ( auth->Acl.count(IdentityPK{_user}), 1u );//...and must not append a duplicate entry.
	}

	TEST( AuthorizeTests, AclUpsertRaisesDenied ){
		auto auth = createAuthorizer();
		auth->AddAcl( _user.Value, PermissionPK{10}, Read | Update, None, _resourcePK );
		auth->AddAcl( _user.Value, PermissionPK{10}, Read | Update, Update, _resourcePK );//denied added on the re-grant.
		ASSERT_EQ( rights(*auth), Read );
	}

	TEST( AuthorizeTests, AclUpsertKeepsSiblingGrant ){
		auto auth = createAuthorizer();
		auth->AddAcl( _user.Value, PermissionPK{10}, Read | Update, None, _resourcePK );
		auth->AddAcl( _user.Value, PermissionPK{11}, Create, None, _resourcePK );
		auth->AddAcl( _user.Value, PermissionPK{10}, Read, None, _resourcePK );//lowering 10 must not disturb 11.
		ASSERT_EQ( rights(*auth), Read | Create );
	}

	TEST( AuthorizeTests, AclRegrantThenRemoveRevokes ){
		auto auth = createAuthorizer();
		auth->AddAcl( _user.Value, PermissionPK{10}, Read, None, _resourcePK );
		auth->AddAcl( _user.Value, PermissionPK{10}, Read, None, _resourcePK );//identical re-grant.
		auth->RemoveAcl( _user.Value, PermissionRole{std::in_place_index<0>, PermissionPK{10}} );
		ASSERT_EQ( auth->Acl.count(IdentityPK{_user}), 0u );//RemoveAcl breaks after the first match - a duplicate would outlive the purge.
		ASSERT_EQ( rights(*auth), None );
	}

	TEST( AuthorizeTests, AclUpsertLowersGroupRights ){
		auto auth = createAuthorizer();
		const GroupPK group{ 280 };
		auth->AddToGroup( group, {_user.Value} );
		auth->AddAcl( group.Value, PermissionPK{10}, Read | Update, None, _resourcePK );
		ASSERT_EQ( rights(*auth), Read | Update );
		auth->AddAcl( group.Value, PermissionPK{10}, Read, None, _resourcePK );//the member must follow the group's lowered grant.
		ASSERT_EQ( rights(*auth), Read );
		ASSERT_EQ( auth->Acl.count(IdentityPK{group}), 1u );
	}

	TEST( AuthorizeTests, AclRoleGrantNotDuplicated ){
		auto auth = createAuthorizer();
		const RolePK role{ 60 };
		auth->Roles.try_emplace( role, Role{role,false} ).first->second.Members.emplace( PermissionRole{std::in_place_index<0>, PermissionPK{10}} );
		auth->Permissions.emplace( PermissionPK{10}, Permission{PermissionPK{10}, _resourcePK, Read, None} );
		auth->AddAcl( _user.Value, role );
		auth->AddAcl( _user.Value, role );//re-granting the same role must not append a second entry either.
		ASSERT_EQ( auth->Acl.count(IdentityPK{_user}), 1u );
		auth->RemoveAcl( _user.Value, PermissionRole{std::in_place_index<1>, role} );
		ASSERT_EQ( rights(*auth), None );
	}

	TEST( AuthorizeTests, PurgeUserSweepsReferences ){
		auto auth = createAuthorizer();
		const GroupPK group{ 290 };
		auth->AddToGroup( group, {_user.Value} );
		auth->AddAcl( _user.Value, PermissionPK{10}, Read, None, _resourcePK );
		ASSERT_EQ( auth->Acl.count(IdentityPK{_user}), 1u );
		ASSERT_TRUE( auth->Groups.find(group)->second.Members.contains(IdentityPK{_user}) );
		auth->PurgeUser( _user );
		ASSERT_EQ( auth->Acl.count(IdentityPK{_user}), 0u );//acl rows and memberships must not outlive the identity.
		ASSERT_FALSE( auth->Groups.find(group)->second.Members.contains(IdentityPK{_user}) );
	}

	TEST( AuthorizeTests, PurgeGroupSweepsReferences ){
		auto auth = createAuthorizer();
		const GroupPK parent{ 300 }, child{ 301 };
		auth->AddToGroup( child, {_user.Value} );
		auth->AddToGroup( parent, {child.Value} );
		auth->AddAcl( child.Value, PermissionPK{10}, Read, None, _resourcePK );
		ASSERT_EQ( rights(*auth), Read );
		auth->PurgeGroup( child );
		ASSERT_EQ( auth->Acl.count(IdentityPK{child}), 0u );
		ASSERT_FALSE( auth->Groups.find(parent)->second.Members.contains(IdentityPK{child}) );
		ASSERT_EQ( rights(*auth), None );//sweeping must not disturb the revocation from #1.
	}

	TEST( AuthorizeTests, PurgeRoleSweepsReferences ){
		auto auth = createAuthorizer();
		const RolePK parent{ 70 }, child{ 71 };
		auth->Roles.try_emplace( child, Role{child,false} ).first->second.Members.emplace( PermissionRole{std::in_place_index<0>, PermissionPK{10}} );
		auth->Roles.try_emplace( parent, Role{parent,false} ).first->second.Members.emplace( PermissionRole{std::in_place_index<1>, child} );
		auth->Permissions.emplace( PermissionPK{10}, Permission{PermissionPK{10}, _resourcePK, Read, None} );
		auth->AddAcl( _user.Value, child );
		ASSERT_EQ( rights(*auth), Read );
		auth->PurgeRole( child );
		ASSERT_EQ( auth->Acl.count(IdentityPK{_user}), 0u );//the acl row named the purged role - swept by value, not by key.
		ASSERT_FALSE( auth->Roles.find(parent)->second.Members.contains(PermissionRole{std::in_place_index<1>, child}) );
		ASSERT_EQ( rights(*auth), None );
	}

	TEST( AuthorizeTests, SystemExecuterConsistent ){
		auto auth = createAuthorizer();
		const UserPK system{ UserPK::System };
		ASSERT_EQ( auth->Rights(_schema, _target, system), All );//all three entry points must agree that System is all-access.
		EXPECT_NO_THROW( auth->Test(_schema, _target, All, system) );
		EXPECT_NO_THROW( auth->TestAdmin(_target, system) );

		const UserPK unknown{ 999 };//...and that an unknown non-System user is not.
		ASSERT_EQ( auth->Rights(_schema, _target, unknown), None );
		EXPECT_THROW( auth->Test(_schema, _target, Read, unknown), Exception );
		EXPECT_THROW( auth->TestAdmin(_target, unknown), Exception );
	}

	//todo.md §12: the IAdminAcl overload returns a pre-completed awaitable any coroutine can co_await - the
	//denial arrives at the co_await, not at the call. VoidTask return: the awaitable dictates no task type.
	Ω testAdminAwait( TestAuthorize& auth, UserPK user, bool& threw, bool& completed )->VoidTask{
		try{
			auto check = auth.TestAdmin( _target, "", user );
			co_await *check;
			threw = false;
		}
		catch( Exception& ){ threw = true; }
		completed = true;
	}
	TEST( AuthorizeTests, TestAdminAwaitable ){
		auto auth = createAuthorizer();
		bool threw{}, completed{};
		testAdminAwait( *auth, UserPK{UserPK::System}, threw, completed );
		ASSERT_TRUE( completed );//pre-completed awaitable, so the coroutine runs synchronously to the end.
		EXPECT_FALSE( threw ) << "System must pass the admin check";
		completed = false;
		testAdminAwait( *auth, UserPK{999}, threw, completed );
		ASSERT_TRUE( completed );
		EXPECT_TRUE( threw ) << "an unknown user must fail the admin check through the awaitable";
	}

	//access-review3 #29:  the fallback called std::to_string( userPK ), which bound PK's then-implicit operator bool and printed "1"
	//for every unknown id - the one identifier the "User not found" response carries.  PK's conversion is explicit now, too.
	TEST( AuthorizeTests, UserNameFallbackPrintsThePk ){
		auto auth = createAuthorizer();
		EXPECT_EQ( "4242", auth->UserName(UserPK{4242}) ); //not in Users - the numeric fallback.
		EXPECT_EQ( "0", auth->UserName(UserPK{0}) );
	}

	TEST( AuthorizeTests, FindResourceBySchemaTarget ){
		auto auth = createAuthorizer();
		let found = auth->FindResource( Resource{jobject{{"schemaName",_schema},{"target",_target}}} );//no pk - resolve from schema/target.
		static_assert( std::is_same_v<decltype(found), const optional<Resource>> ); //access-review3 #19: a copy, not a pointer into Resources that outlives the lock.
		ASSERT_TRUE( found );
		ASSERT_EQ( found->PK, _resourcePK );
		EXPECT_FALSE( auth->FindResource(Resource{ResourcePK{99}, {}}) ); //unknown pk, no schema/target to fall back on.
	}

	TEST( AuthorizeTests, GroupCycleGuards ){
		auto auth = createAuthorizer();
		const GroupPK a{ 300 }, b{ 301 };//cycle in existing data - bypasses TestAddGroupMember.
		auth->Groups.try_emplace( a, Group{a,false} ).first->second.Members.emplace( IdentityPK{b} );
		auto& groupB = auth->Groups.try_emplace( b, Group{b,false} ).first->second;
		groupB.Members.emplace( IdentityPK{a} );
		groupB.Members.emplace( IdentityPK{_user} );
		EXPECT_NO_THROW( auth->TestAddGroupMember( GroupPK{999}, {a.Value} ) );//IsChild traverses the cycle.
		auth->AddAcl( a.Value, PermissionPK{10}, Read, None, _resourcePK );//RecursiveUsers+AddPermission traverse the cycle.
		ASSERT_EQ( rights(*auth), Read );
		auth->DeleteGroup( a );
		ASSERT_EQ( rights(*auth), None );
	}

	TEST( AuthorizeTests, RoleCycleGuards ){
		auto auth = createAuthorizer();
		const RolePK r1{ 50 }, r2{ 51 };//cycle in existing data - bypasses TestAddRoleMember.
		auth->Roles.try_emplace( r1, Role{r1,false} ).first->second.Members.emplace( PermissionRole{std::in_place_index<1>, r2} );
		auto& role2 = auth->Roles.try_emplace( r2, Role{r2,false} ).first->second;
		role2.Members.emplace( PermissionRole{std::in_place_index<1>, r1} );
		role2.Members.emplace( PermissionRole{std::in_place_index<0>, PermissionPK{10}} );
		auth->Permissions.emplace( PermissionPK{10}, Permission{PermissionPK{10}, _resourcePK, Read, None} );
		EXPECT_NO_THROW( auth->TestAddRoleMember( RolePK{52}, r1 ) );//isChild traverses the cycle.
		auth->AddAcl( _user.Value, r1 );//role walk traverses the cycle to reach permission 10.
		ASSERT_EQ( rights(*auth), Read );
	}
}

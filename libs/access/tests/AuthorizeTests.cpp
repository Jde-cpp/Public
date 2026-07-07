#include "gtest/gtest.h"
#include <jde/fwk/io/json.h>
#include <jde/access/Authorize.h>

#define let const auto
namespace Jde::Access::Tests{
	using enum ERights;
	//in-memory unit tests - no db/ql. exercises the recalculation logic directly.
	struct TestAuthorize final : Authorize{
		TestAuthorize()ι:Authorize{"AuthorizeTests"}{}
		using Authorize::AddAcl;
		using Authorize::AddToGroup;
		using Authorize::CreateResource;
		using Authorize::CreateUser;
		using Authorize::DeleteGroup;
		using Authorize::Groups;
		using Authorize::Permissions;
		using Authorize::RemoveAcl;
		using Authorize::RemoveFromGroup;
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

	TEST( AuthorizeTests, FindResourceBySchemaTarget ){
		auto auth = createAuthorizer();
		let found = auth->FindResource( Resource{jobject{{"schemaName",_schema},{"target",_target}}} );//no pk - resolve from schema/target.
		ASSERT_TRUE( found );
		ASSERT_EQ( found->PK, _resourcePK );
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

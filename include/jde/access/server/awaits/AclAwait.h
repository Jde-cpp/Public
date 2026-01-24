#pragma once
#include <jde/db/awaits/ExecuteAwait.h>
#include <jde/db/awaits/ScalerAwait.h>
#include <jde/access/usings.h>
#include <jde/ql/QLAwait.h>

namespace Jde::QL{ struct MutationQL; struct TableQL; }
namespace Jde::Access::Server{
	struct AclQLAwait final : TAwait<jvalue>{
		AclQLAwait( QL::MutationQL m, UserPK executer, SL sl )ι:
			TAwait<jvalue>{ sl },
			_mutation{ move(m) },
			_executer{ executer }
		{}
		α Suspend()ι->void override;
		α InsertAcl()ι->void;
	private:
		QL::MutationQL _mutation;
		Jde::UserPK _executer;

		α Table()ε->const DB::View&;
		α InsertPermission( const jobject& permission )ι->DB::ScalerAwait<optional<ResourcePK>>::Task;
		α InsertPermission( ERights allowed, ERights denied, ResourcePK resourcePK )ι->DB::ScalerAwait<PermissionPK>::Task;
		α InsertRole()ι->DB::ExecuteAwait::Task;
		α PurgeAcl()ι->QL::QLAwait<jobject>::Task;
		α PurgeAcl( IdentityPK::Type identityPK, PermissionPK permissionPK )ι->DB::ExecuteAwait::Task;
	};

	struct AclQLSelectAwait final : TAwait<jvalue>{
		AclQLSelectAwait( const QL::TableQL& ql, UserPK executer, SL sl )ι:
			TAwait<jvalue>{ sl },
			Query{ ql },
			_executer{ executer }
		{}
		α Suspend()ι->void;
	private:
		α GetStatement( const QL::TableQL& childTable, sp<DB::Column> joinColumn )ε->optional<DB::Statement>;
		α LoadRoles( const QL::TableQL& permissionRightsQL )ι->DB::SelectAwait::Task;
		α LoadPermissionRights( const QL::TableQL& permissionRightsQL )ι->DB::SelectAwait::Task;
		α LoadPermissions( const QL::TableQL& permissionsQL )ι->DB::SelectAwait::Task;
		α LoadIdentities( const QL::TableQL& identitiesQL )ι->DB::SelectAwait::Task;
		QL::TableQL Query;
		Jde::UserPK _executer;
	};

	/*
	struct AclHook final : QL::IQLHook{
		α Select( const QL::TableQL& ql, UserPK userPK, SL sl )ι->HookResult override;
		α PurgeBefore( const QL::MutationQL&, UserPK, SL sl )ι->HookResult  override;
		α InsertBefore( const QL::MutationQL& m, UserPK userPK, SL sl )ι->HookResult override;
	};
	*/
}
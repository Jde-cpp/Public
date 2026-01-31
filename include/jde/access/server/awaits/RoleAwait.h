#pragma once
#include <jde/db/awaits/ExecuteAwait.h>
//#include <jde/ql/QLHook.h>
#include <jde/ql/QLAwait.h>

namespace Jde::DB{ struct Statement; }
namespace Jde::QL{ struct MutationQL; struct TableQL; }
namespace Jde::Access::Server{
	struct RoleAwait final : TAwait<jvalue>, noncopyable{
		RoleAwait( const QL::TableQL& q, UserPK userPK, SRCE )ε;
		α Suspend()ι->void override{ Select(); }
		sp<DB::View> MemberTable;
		QL::TableQL Query;
		Jde::UserPK UserPK;
	private:
		α PermissionsStatement( QL::TableQL& permissionQL )ε->optional<DB::Statement>;
		α RoleStatement( QL::TableQL& roleQL )ε->optional<DB::Statement>;
		α Select()ι->QL::QLAwait<>::Task;
	};

	struct RoleMAwait final : TAwait<jvalue>{
		RoleMAwait( const QL::MutationQL& m, UserPK userPK, SRCE )ι:TAwait<jvalue>{ sl }, _mutation{m}, _userPK{userPK}{}
		α Suspend()ι->void override{ if(_mutation.Type==QL::EMutationQL::Remove) Remove(); else Add(); }
	private:
		α Add()ι->void;
		α AddRole( RolePK parentRolePK, const jobject& childRole )ι->DB::ExecuteAwait::Task;
		α AddPermission( RolePK parentRolePK, const jobject& permissionRights )ι->TAwait<PermissionRightsPK>::Task;
		α Remove()ι->DB::ExecuteAwait::Task;

		QL::MutationQL _mutation;
		UserPK _userPK;
	};
}
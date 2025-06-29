drop procedure if exists access_role_purge;
go

create procedure access_role_purge( _role_id int unsigned )
begin
	delete from access_permission_rights where permission_id in ( select member_id from access_role_members where role_id=_role_id );
	delete from access_role_members where role_id=_role_id;
	delete from access_roles where role_id=_role_id;
	delete from access_permissions
	where permission_id not in ( select member_id from access_role_members where role_id=_role_id )
		and permission_id not in ( select permission_id from access_permission_rights )
		and permission_id not in ( select role_id from access_roles );
end

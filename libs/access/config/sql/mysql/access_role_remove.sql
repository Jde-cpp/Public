drop procedure if exists access_role_remove;
go

create procedure access_role_remove( _role_id int unsigned, _permission_id int unsigned )
begin
	if not exists( select 1 from access_role_members where role_id=_role_id and member_id=_permission_id ) then
		signal sqlstate '45000' set message_text = 'Permission is not a member of the role.';
	end if;
	delete from access_permission_rights where permission_id=_permission_id and exists( select 1 from access_role_members where role_id=_role_id and member_id=_permission_id );
	delete from access_role_members where member_id=_permission_id and role_id=_role_id;
	delete from access_permissions where permission_id=_permission_id
		and not exists( select 1 from access_role_members where member_id=_permission_id )
		and not exists( select 1 from access_acl where permission_id=_permission_id );
end

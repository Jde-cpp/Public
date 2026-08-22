create or alter proc [dbo].access_role_remove( @role_id int, @permission_id int ) as begin
	if not exists( select 1 from access_role_members where role_id=@role_id and member_id=@permission_id )
		throw 50000, 'Permission is not a member of the role.', 1;
	delete from access_permission_rights where permission_id=@permission_id and exists( select 1 from access_role_members where role_id=@role_id and member_id=@permission_id );
	delete from access_role_members where member_id=@permission_id and role_id=@role_id;
	delete from access_permissions where permission_id=@permission_id
		and not exists( select 1 from access_role_members where member_id=@permission_id )
		and not exists( select 1 from access_acl where permission_id=@permission_id );
end

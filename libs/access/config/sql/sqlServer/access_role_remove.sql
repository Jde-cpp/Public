create or alter proc [dbo].access_role_remove( @role_id int, @permission_id int ) as begin
	delete from access_permission_rights where permission_id=@permission_id;
	delete from access_role_members where member_id=@permission_id and role_id=@role_id;
	delete from access_permissions where permission_id=@permission_id;
end
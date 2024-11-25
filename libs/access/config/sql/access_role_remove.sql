drop procedure if exists access_role_remove;
go

#DELIMITER $$
create procedure access_role_remove( _role_id int unsigned, _permission_id int unsigned )
begin
	delete from access_permission_rights where permission_id=_permission_id;
	delete from access_role_members where member_id=_permission_id and role_id=_role_id;
	delete from access_permissions where permission_id=_permission_id;
end
#$$
#DELIMITER ;
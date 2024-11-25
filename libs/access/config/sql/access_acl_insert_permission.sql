drop procedure if exists access_acl_insert_permission;
go

#DELIMITER $$
create procedure access_acl_insert_permission( _identityId int unsigned, _allowed tinyint unsigned, _denied tinyint unsigned, _resourceId int unsigned )
begin
	declare _permission_id int unsigned;
	insert into access_permissions( is_role ) values( false );
	set _permission_id = LAST_INSERT_ID();
	insert into access_permission_rights( permission_id, resource_id, allowed, denied ) values( _permission_id, _resourceId, _allowed, _denied );
	insert into access_acl( identity_id, permission_id ) values( _identityId, _permission_id );
	select _permission_id;
end
#$$
#DELIMITER ;
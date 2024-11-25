drop procedure if exists access_role_add;
go

#DELIMITER $$
create procedure access_role_add( _role_id int unsigned, _allowed tinyint unsigned, _denied tinyint unsigned, _resourceTarget varchar(256) )
begin
	declare _permission_id int unsigned;
	insert into access_permissions( is_role ) values( false );
	set _permission_id = LAST_INSERT_ID();

	insert into access_permission_rights( permission_id, allowed, denied, resource_id )
	values( _permission_id, _allowed, _denied, ( select resource_id from access_resources where target = _resourceTarget ) );

	insert into access_role_members( role_id, member_id ) values( _role_id, _permission_id );
	select _permission_id;
end
#$$
#DELIMITER ;
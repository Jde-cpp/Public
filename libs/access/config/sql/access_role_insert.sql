drop procedure if exists access_role_insert;
go

#DELIMITER $$
create procedure access_role_insert( _name varchar(256), _target varchar(256), _attributes smallint unsigned, _description varchar(2048) )
begin
	declare _role_id int unsigned;
	insert into access_permissions( is_role ) values( true );
	set _role_id = LAST_INSERT_ID();

	insert into access_roles( role_id, name, target, attributes, description ) values( _role_id, _name, _target, _attributes, _description );
	select _role_id;
end
#$$
#DELIMITER ;
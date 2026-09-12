drop procedure if exists access_role_insert;
go

create procedure access_role_insert( _name varchar(256), _slug varchar(256), _attributes smallint unsigned, _description varchar(2048), out _role_id int unsigned )
begin
	insert into access_permissions( is_role ) values( true );
	set _role_id = LAST_INSERT_ID();

	insert into access_roles( role_id, name, slug, attributes, description ) values( _role_id, _name, _slug, _attributes, _description );
end

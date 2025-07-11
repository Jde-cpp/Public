drop procedure if exists access_role_add;
go

create procedure access_role_add( _role_id int unsigned, _allowed tinyint unsigned, _denied tinyint unsigned, _resourceTarget varchar(256), out _permission_id int unsigned )
begin
	select permission_id
	into _permission_id
	from access_role_members members
		join access_permission_rights permissions on members.member_id=permissions.permission_id
		join access_resources resources using(resource_id)
	where members.role_id=_role_id
		and resources.target=_resourceTarget;

	if _permission_id is not null then
		update access_permission_rights set allowed=_allowed, denied=_denied
		where permission_id=_permission_id;
	else
		insert into access_permissions( is_role ) values( false );
		set _permission_id = LAST_INSERT_ID();

		insert into access_permission_rights( permission_id, allowed, denied, resource_id )
		values( _permission_id, _allowed, _denied, ( select resource_id from access_resources where target = _resourceTarget and criteria is null ) );

		insert into access_role_members( role_id, member_id ) values( _role_id, _permission_id );
	end if;
end

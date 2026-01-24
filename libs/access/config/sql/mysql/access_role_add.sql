drop procedure if exists access_role_add;
go

create procedure access_role_add( _role_id int unsigned, _allowed tinyint unsigned, _denied tinyint unsigned, _resourceTarget varchar(64), _schema varchar(256), _resourceName varchar(64), _criteria varchar(832), out _permission_id int unsigned )
begin
	declare _resource_id smallint unsigned;
	select resource_id
	into _resource_id
	from access_resources
	where target=_resourceTarget
		and schema_name = coalesce(_schema, schema_name)
		and criteria = coalesce(_criteria, criteria);
	if _resource_id is null then
		insert into access_resources( target, schema_name, name, criteria ) values( _resourceTarget, _schema, _resourceName, _criteria );
		set _resource_id = LAST_INSERT_ID();
	end if;
	select permission_id
	into _permission_id
	from access_role_members members
		join access_permission_rights permissions on members.member_id=permissions.permission_id
		join access_resources resources using(resource_id)
	where members.role_id=_role_id
		and resources.resource_id=_resource_id;

	if _permission_id is not null then
		update access_permission_rights set allowed=_allowed, denied=_denied
		where permission_id=_permission_id;
	else
		insert into access_permissions( is_role ) values( false );
		set _permission_id = LAST_INSERT_ID();

		insert into access_permission_rights( permission_id, allowed, denied, resource_id )
		values( _permission_id, _allowed, _denied, _resource_id );

		insert into access_role_members( role_id, member_id ) values( _role_id, _permission_id );
	end if;
end

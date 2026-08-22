drop procedure if exists access_ac_upsert_permission;
go

create procedure access_ac_upsert_permission( _identityId int unsigned, _allowed bigint unsigned, _denied bigint unsigned, _resourceId int unsigned, out _permission_id int unsigned )
begin
	set _permission_id = (
		select max(permission_id)
		from access_acl
			join access_permission_rights using(permission_id)
		where resource_id=_resourceId
			and identity_id=_identityId
	);
	if _permission_id is null then
		insert into access_permissions( is_role ) values( false );
		set _permission_id = LAST_INSERT_ID();
		insert into access_permission_rights( permission_id, resource_id, allowed, denied ) values( _permission_id, _resourceId, _allowed, _denied );
		insert into access_acl( identity_id, permission_id ) values( _identityId, _permission_id );
	else
		update access_permission_rights set allowed=_allowed, denied=_denied where permission_id=_permission_id;
	end if;
end

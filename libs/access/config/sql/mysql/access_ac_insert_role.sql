drop procedure if exists access_ac_insert_role;
go

create procedure access_ac_insert_role( _identityId int unsigned, _role_id int unsigned )
begin
	insert into access_acl( identity_id, permission_id ) values( _identityId, _role_id );
end

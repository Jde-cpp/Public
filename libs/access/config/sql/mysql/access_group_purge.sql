drop procedure if exists access_group_purge;
go

create procedure access_group_purge( _identity_id int unsigned )
begin
	delete from access_acl where identity_id=_identity_id;
	delete from `access_groups` where identity_id=_identity_id or member_id=_identity_id;
	delete from access_identities where identity_id=_identity_id;
end

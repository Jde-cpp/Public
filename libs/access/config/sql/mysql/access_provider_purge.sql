drop procedure if exists access_provider_purge;
go

#DELIMITER $$
create procedure access_provider_purge( _provider_id int unsigned )
begin
	delete from access_users where identity_id in ( select identity_id from access_identities where provider_id = _provider_id );
	delete from access_identities where provider_id = _provider_id;
	delete from access_providers where provider_id = _provider_id;
end
#$$
#DELIMITER ;
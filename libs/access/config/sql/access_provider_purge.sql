drop procedure if exists access_provider_purge;
go

#DELIMITER $$
create procedure access_provider_purge( _provider_id int unsigned )
begin
	delete from access_users where entity_id in ( select id from access_entities where provider_id = _provider_id );
	delete from access_entities where provider_id = _provider_id;
	delete from access_providers where id = _provider_id;
end
#$$
#DELIMITER ;
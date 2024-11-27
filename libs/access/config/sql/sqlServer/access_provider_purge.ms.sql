create or alter proc access_provider_purge( @provider_id int ) as
begin
	delete from access_users where entity_id in ( select id from access_entities where provider_id = @provider_id );
	delete from access_entities where provider_id = @provider_id;
	delete from access_providers where id = @provider_id;
end
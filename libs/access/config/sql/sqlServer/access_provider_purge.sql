create or alter proc access_provider_purge( @provider_id int ) as begin
	delete from access_users where identity_id in ( select identity_id from access_identities where provider_id = @provider_id );
	delete from access_identities where provider_id = @provider_id;
	delete from access_providers where provider_id = @provider_id;
end
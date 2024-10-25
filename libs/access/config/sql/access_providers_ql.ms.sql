create or alter VIEW access_providers_ql AS
	select access_providers.id, name
	from access_providers join access_provider_types on access_providers.provider_type_id = access_provider_types.id
	where access_provider_types.name != 'OpcServer'
	union
	select access_providers.id, target as name
	from access_providers join access_provider_types on access_providers.provider_type_id = access_provider_types.id
	where access_provider_types.name = 'OpcServer';

create or alter view access_providers_ql as
	select providers.provider_id, name
	from access_providers providers join access_provider_types types on providers.provider_type_id = types.provider_type_id
	where types.name != 'OpcServer'
	union
	select providers.provider_id, target as name
	from access_providers providers join access_provider_types types on providers.provider_type_id = types.provider_type_id
	where types.name = 'OpcServer';
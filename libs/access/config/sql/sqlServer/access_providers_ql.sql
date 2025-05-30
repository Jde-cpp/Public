create or alter view [dbo].access_providers_ql as
	select providers.provider_id, name
	from [dbo].access_providers providers
		join [dbo].access_provider_types types on providers.provider_type_id = types.provider_type_id
	where types.name != 'OpcServer'
	union
	select providers.provider_id, target as name
	from [dbo].access_providers providers
		join [dbo].access_provider_types types on providers.provider_type_id = types.provider_type_id
	where types.name = 'OpcServer';
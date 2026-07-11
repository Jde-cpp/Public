drop view if exists access_providers_ql;
GO
create view access_providers_ql as
	select access_providers.provider_id, name
	from access_providers join access_provider_types using(provider_type_id)
	where access_provider_types.name != 'OpcServer'
	union
	select access_providers.provider_id, target as name
	from access_providers join access_provider_types using(provider_type_id)
	where access_provider_types.name = 'OpcServer';

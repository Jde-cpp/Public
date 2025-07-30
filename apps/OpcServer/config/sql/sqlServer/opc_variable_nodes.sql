create or alter view [dbo].opc_variable_nodes as
select n.server_id, n.node_id, n.ns, n.number, n.string, n.guid, n.bytes, n.namespace_uri, n.server_index, n.is_global,
	a.parent_node_id, a.ref_type_id, a.browse_id, a.type_def_id, a.specified, a.name, a.description, a.write_mask, a.user_write_mask, a.created, a.deleted, a.updated,
	a.data_type_id, a.value_rank, a.array_dims, a.access_level, a.user_access_level, a.minimum_sampling_interval, a.historizing,
	a.variant_id, v.data_type_id variant_data_type_id, v.array_dims variant_array_dims
from [dbo].opc_server_node_ids n
	join [dbo].opc_variables a on n.node_id = a.node_id
	left join [dbo].opc_variants v on a.variant_id = v.variant_id
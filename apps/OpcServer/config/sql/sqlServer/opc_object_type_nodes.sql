create or alter view [dbo].opc_object_type_nodes as
select n.server_id, n.node_id, n.ns, n.number, n.string, n.guid, n.bytes, n.namespace_uri, n.server_index, n.is_global,
	a.parent_node_id, a.browse_id, a.ref_type_id, a.specified, a.name, a.description, a.write_mask, a.user_write_mask, a.created, a.deleted, a.updated,
	a.is_abstract
from [dbo].opc_server_node_ids n
	join [dbo].opc_object_types a on n.node_id = a.node_id
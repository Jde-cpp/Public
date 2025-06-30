create or replace view opc_object_nodes as
select n.server_id, node_id, n.ns, n.number, n.string, n.guid, n.bytes, n.namespace_uri, n.server_index, n.is_global,
	a.parent_node_id, a.ref_type_id, a.browse_id, a.type_def_id, a.specified, a.name, a.description, a.write_mask, a.user_write_mask, a.created, a.deleted, a.updated,
	a.event_notifier
from opc_server_node_ids n
	join opc_objects a using(node_id)
create or alter proc [dbo].opc_node_insert(
		@node_id bigint, @parent_node_id bigint, @ref_id bigint, @type_id bigint,
		@obj_attr_id bigint, @object_type_attr_id bigint, @variable_attr_id bigint,
		@browse_id int)
as begin
	set nocount on;
	insert into [dbo].opc_nodes(node_id, parent_node_id, reference_type_id, type_definition_id, object_attr_id, object_type_attr_id, variable_attr_id, browse_id)
	values( @node_id, @parent_node_id, @ref_id, @type_id, @obj_attr_id, @object_type_attr_id, @variable_attr_id, @browse_id );
end;
create or alter proc [dbo].opc_object_insert(
		@ns smallint, @number int, @string varchar(256), @guid binary(16), @bytes varbinary(256),
		@parent_node_id bigint, @ref_type_id bigint, @type_def_id bigint, @browse_id int,
		@specified int, @name varchar(256), @description varchar(2048), @write_mask int, @user_write_mask int,
		@event_notifier tinyint,
		@node_id bigint output)
as begin
	set nocount on;
	exec [dbo].opc_node_id_insert @ns, @number, @string, @guid, @bytes, null, null, true, @node_id output;

	insert into [dbo].opc_objects( node_id, parent_node_id, ref_type_id, type_def_id, browse_id, specified, name, description, write_mask, user_write_mask, event_notifier )
	values( @node_id, @parent_node_id, @ref_type_id, @type_def_id, @browse_id, @specified, @name, @description, @write_mask, @user_write_mask, @event_notifier );
end;
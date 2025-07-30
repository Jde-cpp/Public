create or alter proc [dbo].opc_variable_insert(
	@ns smallint, @number int, @string varchar(256), @guid binary(16), @bytes varbinary(256),
	@parent_node_id bigint, @ref_type_id bigint, @type_id bigint, @browse_id int,
	@specified int, @name varchar(256), @description varchar(2048), @write_mask int, @user_write_mask int,
	@variant_id int, @data_type_id bigint, @value_rank int, @array_dims varchar(256), @access_level tinyint, @user_access_level tinyint, @minimum_sampling_interval float, @historizing bit,
	@node_id bigint output )
as begin
	set nocount on;
	declare @is_node bit;
	set @is_node = 1;
	if( @data_type_id=0 )
		raiserror( 'Data type ID cannot be zero', 16, 1 );

--	set @is_node = (select @number is not null or @string is not null or @guid is not null or @bytes is not null);
	set @is_node = ( select iif(@number is null and @string is null and @guid is null and @bytes is null, 0, 1) );
	if( @is_node=1 )
		exec [dbo].opc_node_id_insert @ns, @number, @string, @guid, @bytes, null, null, null, @node_id output;
	else
		set @node_id = null;
	if( @data_type_id<=32750 and (select count(*) from opc_node_ids where node_id=@data_type_id)=0 )
		exec [dbo].opc_node_id_insert 0, @data_type_id, null, null, null, null, null, null, @data_type_id output;

	insert into [dbo].opc_variables( node_id, parent_node_id, ref_type_id, type_def_id, browse_id, specified, name,description,write_mask,user_write_mask,
		variant_id,data_type_id,value_rank,array_dims,access_level,user_access_level,minimum_sampling_interval,historizing )
		values( @node_id, @parent_node_id, @ref_type_id, @type_id, @browse_id, @specified, @name, @description, @write_mask, @user_write_mask,
		@variant_id,@data_type_id,@value_rank,@array_dims,@access_level,@user_access_level,@minimum_sampling_interval,@historizing );
end
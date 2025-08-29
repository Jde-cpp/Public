create or alter proc [dbo].opc_variant_insert( @data_type_id bigint, @array_dims varchar(256), @variant_id int output ) as begin
	set nocount on;
	if( (select count(*) from opc_node_ids where node_id=@data_type_id)=0 and @data_type_id<=32750 )
		exec [dbo].opc_node_id_insert 0, @data_type_id, null, null, null, null, null, null, @data_type_id output;

	insert into [dbo].opc_variants( data_type_id,array_dims )
	values( @data_type_id,@array_dims );

	set @variant_id = scope_identity();
end
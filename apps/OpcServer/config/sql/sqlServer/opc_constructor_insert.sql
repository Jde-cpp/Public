create or alter proc [dbo].opc_constructor_insert( @node_id bigint, @browse_id int, @variant_id int ) as begin
	set nocount on;
	insert into [dbo].opc_constructors( node_id, browse_id, variant_id )
	values( @node_id, @browse_id, @variant_id );
end;
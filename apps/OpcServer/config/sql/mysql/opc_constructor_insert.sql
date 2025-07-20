drop procedure if exists opc_constructor_insert;
GO

create procedure opc_constructor_insert( _node_id bigint unsigned, _browse_id int unsigned, _variant_id int unsigned ) begin
	insert into opc_constructors( node_id, browse_id, variant_id )
	values( _node_id, _browse_id, _variant_id );
end;
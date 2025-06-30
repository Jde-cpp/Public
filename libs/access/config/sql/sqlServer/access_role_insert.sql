create or alter proc [dbo].access_role_insert( @name varchar(256), @target varchar(256), @attributes smallint, @description varchar(2048), @role_id int output ) as begin
	set nocount on;
	insert into access_permissions( is_role ) values( 1 );
	set @role_id = scope_identity();

	insert into access_roles( role_id, name, target, attributes, description ) values( @role_id, @name, @target, @attributes, @description );
end
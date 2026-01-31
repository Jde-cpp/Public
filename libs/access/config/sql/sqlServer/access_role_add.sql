create or alter proc [dbo].access_role_add( @role_id int, @allowed tinyint, @denied tinyint, @resourceTarget varchar(256), @schema varchar(256), @resourceName varchar(64), @criteria varchar(832), @permission_id int output ) as begin
	set nocount on;
	declare @resource_id smallint;
	select @resource_id = resource_id
	from access_resources
	where target=@resourceTarget
		and schema_name = coalesce(@schema, schema_name)
		and criteria is not distinct from @criteria;
	if @resource_id is null begin
		insert into access_resources( target, schema_name, name, criteria ) values( @resourceTarget, @schema, @resourceName, @criteria );
		set @resource_id = scope_identity();
	end;

	select @permission_id=permission_id
	from access_role_members members
		join access_permission_rights rights on members.member_id=rights.permission_id
	where members.role_id=@role_id
		and rights.resource_id=@resource_id;

	if @permission_id is not null begin
		update access_permission_rights set allowed=@allowed, denied=@denied
		where permission_id=@permission_id;
	end else begin
		insert into access_permissions( is_role ) values( 0 );
		set @permission_id = scope_identity();

		insert into access_permission_rights( permission_id, allowed, denied, resource_id )
		values( @permission_id, @allowed, @denied, @resource_id );

		insert into access_role_members( role_id, member_id ) values( @role_id, @permission_id );
	end;
end
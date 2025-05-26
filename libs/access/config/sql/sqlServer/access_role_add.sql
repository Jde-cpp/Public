create or alter proc access_role_add( @role_id int, @allowed tinyint, @denied tinyint, @resourceTarget varchar(256) ) as begin
	set nocount on;
	declare @permission_id int;

	select @permission_id=permission_id
	from access_role_members members
		join access_permission_rights permissions on members.member_id=permissions.permission_id
		join access_resources resources on permissions.resource_id = resources.resource_id
	where members.role_id=@role_id
		and resources.target=@resourceTarget;

	if @permission_id is not null begin
		update access_permission_rights set allowed=@allowed, denied=@denied
		where permission_id=@permission_id;
	end else begin
		insert into access_permissions( is_role ) values( 0 );
		set @permission_id = scope_identity();

		insert into access_permission_rights( permission_id, allowed, denied, resource_id )
		values( @permission_id, @allowed, @denied, ( select resource_id from access_resources where target = @resourceTarget and criteria is null ) );

		insert into access_role_members( role_id, member_id ) values( @role_id, @permission_id );
	end;
	select @permission_id;
end
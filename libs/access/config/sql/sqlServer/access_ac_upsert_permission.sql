create or alter proc access_ac_upsert_permission( @identityId int, @allowed tinyint, @denied tinyint, @resourceId int ) as
begin
	set nocount on;
	declare @permission_id int;
	set @permission_id = (
		select max( acl.permission_id )
		from access_acl acl
			join access_permission_rights pr on acl.permission_id=pr.permission_id
			join access_resources r on pr.resource_id=r.resource_id
		where pr.resource_id=@resourceId
			and identity_id=@identityId
			and criteria is null
	);
	if @permission_id is null begin
		insert into access_permissions( is_role ) values( 0 );
		set @permission_id = scope_identity();
		insert into access_permission_rights( permission_id, resource_id, allowed, denied ) values( @permission_id, @resourceId, @allowed, @denied );
		insert into access_acl( identity_id, permission_id ) values( @identityId, @permission_id );
	end else
		update access_permission_rights set allowed=@allowed, denied=@denied where permission_id=@permission_id;

	select @permission_id;
end
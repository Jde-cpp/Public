create or alter proc [dbo].app_instance_tag_level_upsert( @instanceId int, @type varchar(256), @tag bigint, @levelId tinyint ) as
begin
	set nocount on;
	if exists( select 1 from [dbo].app_instance_tag_levels where instance_id=@instanceId and tag=@tag and type=@type )
		update [dbo].app_instance_tag_levels
		set level_id=@levelId
		where instance_id=@instanceId and tag=@tag and type=@type;
	else
		insert into [dbo].app_instance_tag_levels(instance_id, type, tag, level_id)
		values(@instanceId, @type, @tag, @levelId);
end
go

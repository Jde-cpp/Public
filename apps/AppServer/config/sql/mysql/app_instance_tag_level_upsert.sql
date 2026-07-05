drop procedure if exists app_instance_tag_level_upsert;
GO
#DELIMITER $$;
CREATE PROCEDURE app_instance_tag_level_upsert( _instance_id int unsigned, _type varchar(256), _tag bigint unsigned, _level_id tinyint unsigned )
begin
	if exists( select 1 from app_instance_tag_levels where instance_id=_instance_id and tag=_tag and type=_type ) then
		update app_instance_tag_levels
		set level_id=_level_id
		where instance_id=_instance_id and tag=_tag and type=_type;
	else
		insert into app_instance_tag_levels(instance_id, type, tag, level_id)
		values(_instance_id, _type, _tag, _level_id);
 end if;
end;
#$$
#DELIMITER ;
create or alter proc access_user_insert_login( @login_name varchar(255), @provider_id int, @provider_target varchar(255) ) as
begin
	declare @entity_id int; declare @provider_name varchar(255);

	if @provider_target is not null begin
		select @provider_id=id from access_providers where target = @provider_target;
		set @provider_name = @provider_target;
	end
	else begin
		if @provider_id is not null begin
			select @provider_name=name from access_provider_types where id = @provider_id;
		end
	end

	if @provider_name is not null
		set @provider_target = CONCAT(@provider_name, '-', @login_name);
	else
		set @provider_target = @login_name;

	exec access_entity_insert @login_name, @provider_target, @provider_id, 0, null, false;
	SET @entity_id = @@IDENTITY;

	insert into access_users(entity_id, login_name) values(@entity_id, @login_name);
	SELECT @entity_id;
end

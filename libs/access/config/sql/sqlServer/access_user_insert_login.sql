create or alter proc [dbo].access_user_insert_login( @login_name varchar(255), @provider_id int, @provider_target varchar(255), @identity_id int output ) as begin
	set nocount on;
	declare @provider_name varchar(255);

	if @provider_target is not null begin
		select @provider_id=provider_id from access_providers where target = @provider_target;
		set @provider_name = @provider_target;
	end
	else begin
		if @provider_id is not null begin
			select @provider_name=name from access_provider_types where provider_type_id = @provider_id;
		end
	end

	if @provider_name is not null
		set @provider_target = CONCAT(@provider_name, '-', @login_name);
	else
		set @provider_target = @login_name;

	exec [dbo].[access_identity_insert] @login_name, @provider_id, @provider_target, null, null, 0, @identity_id output;

	insert into access_users(identity_id, login_name) values(@identity_id, @login_name);
end
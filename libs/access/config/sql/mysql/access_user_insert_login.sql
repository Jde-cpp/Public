drop procedure if exists access_user_insert_login;
go

#DELIMITER $$
create procedure access_user_insert_login( _login_name varchar(255), provider_id int unsigned,_provider_target varchar(255) )
begin
	declare entity_id int unsigned; declare provider_target varchar(255); declare provider_name varchar(255); declare provider_id int unsigned;

	if( _provider_target is not null ) then
		select id into provider_id from access_providers where target = _provider_target;
		set provider_name = _provider_target;
	else
		set provider_id = provider_id;
		if( provider_id is not null ) then
			select name into provider_name from access_provider_types where id = provider_id;
		end if;
	end if;

	if( provider_name is not null ) then
		set provider_target = CONCAT(provider_name, '-', _login_name);
	else
		set provider_target = _login_name;
	end if;
	CALL access_entity_insert(_login_name, 0, provider_target, null, false, provider_id);
	SET entity_id = LAST_INSERT_ID();

	insert into access_users(entity_id, login_name) values(entity_id, _login_name);
	SELECT entity_id;
end
#$$
#DELIMITER ;
#call access_user_insert_login( 'a', 1, null );
drop procedure if exists access_user_insert_login;
go

create procedure access_user_insert_login( _login_name varchar(255), _provider_id int unsigned,_provider_target varchar(255), out _identity_id int unsigned )
begin
	 declare provider_target varchar(255); declare provider_name varchar(255);

	if( _provider_target is not null ) then
		select provider_id into _provider_id from access_providers where target = _provider_target;
		set provider_name = _provider_target;
	elseif( _provider_id is not null ) then
		select name into provider_name from access_provider_types where provider_type_id = _provider_id;
	end if;

	if( provider_name is not null ) then
		set provider_target = CONCAT(provider_name, '-', _login_name);
	else
		set provider_target = _login_name;
	end if;
	CALL access_identity_insert(_login_name, _provider_id, provider_target, null, null, false, _identity_id);
	insert into access_users(identity_id, login_name) values(_identity_id, _login_name);
end
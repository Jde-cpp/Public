drop procedure if exists access_user_insert;
go

create procedure access_user_insert( _identity_id int unsigned, _login_name varchar(255), _password varchar(2047), _modulus varchar(1024), _exponent int unsigned )
begin
	insert into access_users( identity_id, login_name, password, modulus, exponent ) values( _identity_id, _login_name, _password, _modulus, _exponent );
end

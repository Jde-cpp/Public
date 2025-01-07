drop procedure if exists access_user_insert_key;
go

#DELIMITER $$
create procedure access_user_insert_key( modulus varchar(1024), exponent int unsigned, provider_id int unsigned, name varchar(255), target varchar(255), description varchar(2047) )
begin
	declare _identity_id int unsigned;
	call access_identity_insert(name, provider_id, target, 0, description, false);
	set _identity_id = LAST_INSERT_ID();

	insert into access_users( identity_id, modulus, exponent ) values( _identity_id, modulus, exponent );
	select _identity_id;
end
#$$
#DELIMITER ;
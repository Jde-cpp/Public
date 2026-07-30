drop procedure if exists access_user_insert_key;
go

create procedure access_user_insert_key( modulus varchar(1024), exponent int unsigned, provider_id int unsigned, name varchar(255), target varchar(255), description varchar(2047), issuer varchar(1024), subject varchar(1024), email varchar(255), expiration datetime, out _identity_id int unsigned )
begin
	if length(name)=0 or length(target)=0 then
		signal sqlstate '45000' set message_text = 'Name and target are required';
	end if;
	call access_identity_insert(name, provider_id, target, 0, description, false, email, _identity_id);
	insert into access_users( identity_id, modulus, exponent, issuer, subject, expiration ) values( _identity_id, modulus, exponent, issuer, subject, expiration );
end
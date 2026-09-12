drop procedure if exists access_user_insert_key;
go

create procedure access_user_insert_key( modulus varchar(1024), exponent int unsigned, provider_id int unsigned, name varchar(255), slug varchar(255), description varchar(2047), issuer varchar(1024), subject_alt varchar(1024), distinguished varchar(1024), email varchar(255), expiration datetime, fingerprint varchar(95), out _identity_id int unsigned )
begin
	declare msg varchar(128);
	if length(name)=0 or length(slug)=0 then
		signal sqlstate '45000' set message_text = 'Name and slug are required';
	end if;
	if exists( select 1 from access_identities i where i.slug = slug ) then
		set msg = concat('Slug ''', left(slug,103), ''' already exists.');
		signal sqlstate '45000' set message_text = msg;
	end if;
	call access_identity_insert(name, provider_id, slug, 0, description, false, email, _identity_id);
	insert into access_users( identity_id, modulus, exponent, issuer, subject_alt, distinguished, expiration, fingerprint ) values( _identity_id, modulus, exponent, issuer, subject_alt, distinguished, expiration, fingerprint );
end
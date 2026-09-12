create or alter procedure [dbo].access_user_insert_key( @modulus varchar(1024), @exponent int, @provider_id int, @name varchar(255), @slug varchar(255), @description varchar(2047), @issuer varchar(1024), @subject_alt varchar(1024), @distinguished varchar(1024), @email varchar(255), @expiration datetime, @fingerprint varchar(95), @identity_id int output ) as
begin
	set nocount on;
	if len(@name)=0 or len(@slug)=0
		throw 50000, 'Name and slug are required', 1;
	if exists( select 1 from access_identities where slug=@slug ) begin
		declare @msg nvarchar(2048) = concat( 'Slug ''', @slug, ''' already exists.' );
		throw 50000, @msg, 1;
	end;
	exec [dbo].[access_identity_insert] @name, @provider_id, @slug, 0, @description, 0, @email, @identity_id output;

	insert into access_users( identity_id, modulus, exponent, issuer, subject_alt, distinguished, expiration, fingerprint ) values( @identity_id, @modulus, @exponent, @issuer, @subject_alt, @distinguished, @expiration, @fingerprint );
end
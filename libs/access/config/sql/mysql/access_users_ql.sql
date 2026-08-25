create or replace view access_users_ql as
select base.identity_id, base.name, base.provider_id, base.target, base.attributes, base.created, base.updated, base.deleted, base.description, base.is_group, base.email,
       users.login_name, users.password, users.modulus, users.exponent, users.issuer, users.subject_alt, users.distinguished, users.expiration
from access_identities base
	left join access_users users using(identity_id)
#access_identities holds both halves;  `users` is the is_group=0 one (access-meta.jsonnet users.identityId criteria).  SchemaDdl strips #-comments, so they never reach the driver.
where base.is_group = 0;
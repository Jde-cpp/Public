create or replace view access_group_members as
select map.identity_id group_id, parents.slug group_slug, map.member_id,
	members.name, members.attributes, members.slug, members.description, members.created, members.updated, members.deleted, members.is_group
from `access_groups` map
	join access_identities parents using(identity_id)
	join access_identities members on map.member_id=members.identity_id
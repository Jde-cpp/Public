create or alter view [dbo].access_group_members as
select map.identity_id group_id, parents.slug group_slug, map.member_id,
	members.name, members.attributes, members.slug, members.description, members.created, members.updated, members.deleted, members.is_group
from [dbo].access_groups map
	join [dbo].access_identities parents on map.identity_id=parents.identity_id
	join [dbo].access_identities members on map.member_id=members.identity_id
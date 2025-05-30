create or alter view [dbo].access_group_members as
select map.identity_id group_id, parents.target group_target, map.member_id,
	members.name, members.attributes, members.target, members.description, members.created, members.updated, members.deleted, members.is_group
from [dbo].access_groupings map
	join [dbo].access_identities parents on map.identity_id=parents.identity_id
	join [dbo].access_identities members on map.member_id=members.identity_id
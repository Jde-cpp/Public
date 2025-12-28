create or alter view [dbo].app_connections_ql as
	select connection_id, i.name instance_name, p.name program_name, h.name host_name, c.created, c.deleted, c.pid
	from [dbo].app_connections c
		join [dbo].app_instances i on c.instance_id=i.instance_id
		join [dbo].app_programs p on i.program_id=p.program_id
		join [dbo].app_hosts h on i.host_id=h.host_id;
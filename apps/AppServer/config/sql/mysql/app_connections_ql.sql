create or replace view app_connections_ql as
	select connection_id, i.name instance_name, p.name program_name, h.name host_name, c.created, c.deleted, c.pid
	from app_connections c
		join app_instances i using(instance_id)
		join app_programs p using(program_id)
		join app_hosts h using(host_id);
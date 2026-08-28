import { Route, Routes } from '@angular/router';

//First match wins, so the duplicated 'access' path resolves like Angular's own matcher.  ':param' segments match anything;
//'**' consumes any remainder, so every prefix inside a node browse path is routable.
export function matchConfig( routes:Routes, segments:string[] ):Route|undefined{
	for( const config of routes ){
		if( config.path=='**' )
			return config;
		const configSegments = (config.path ?? '').split('/').filter( s=>s.length );
		if( configSegments.length>segments.length || !configSegments.every( (cs,j)=>cs.startsWith(':') || cs==segments[j] ) )
			continue;
		if( configSegments.length==segments.length )
			return config;
		const child = config.children ? matchConfig( config.children, segments.slice(configSegments.length) ) : undefined;
		if( child )
			return child;
	}
	return undefined;
}

//How many of `segments` a matching route spells out literally - -1 when nothing matches.  ':param' routes accept anything, so
//when several candidate urls match ('/gateways/users/alice' fits gateways/:gateway/:connection) the one with the most literal
//segments is the one the writer meant ('/access/users/alice').
export function matchLiterals( routes:Routes, segments:string[] ):number{
	for( const config of routes ){
		if( config.path=='**' )
			return 0;
		const configSegments = (config.path ?? '').split('/').filter( s=>s.length );
		if( configSegments.length>segments.length || !configSegments.every( (cs,j)=>cs.startsWith(':') || cs==segments[j] ) )
			continue;
		const literals = configSegments.filter( cs=>!cs.startsWith(':') ).length;
		if( configSegments.length==segments.length )
			return literals;
		const child = config.children ? matchLiterals( config.children, segments.slice(configSegments.length) ) : -1;
		if( child>=0 )
			return literals+child;
	}
	return -1;
}

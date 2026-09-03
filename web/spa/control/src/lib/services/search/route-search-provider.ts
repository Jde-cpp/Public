import { inject, Injectable } from '@angular/core';
import { Router, Routes } from '@angular/router';
import { RouteStore } from '../route-store';
import { matchLiterals } from '../route-utils';
import { ISearchProvider, searchRank, SearchResult } from './search-provider';

type CollectionItem = string | { path:string; title?:string; data?:{ icon?:string; summary?:string } };

//Router items:  the static route tree (titles, data.summary/icon) plus the children the pages discovered at runtime and parked
//in RouteStore - gateways, connections, app instances, access rows.  No prefix.
@Injectable({ providedIn: 'root' })
export class RouteSearchProvider implements ISearchProvider{
	readonly name = 'routes';
	#router = inject( Router );
	#routeStore = inject( RouteStore );

	async search( text:string, scope:string|undefined, limit:number ):Promise<SearchResult[]>{
		if( scope || !text.length )
			return [];
		const items = this.items();
		return items
			.map( item=>({ ...item, rank: searchRank(text, item.title, item.summary) }) )
			.filter( item=>item.rank<3 )
			.sort( (a,b)=>a.rank-b.rank || a.title.localeCompare(b.title) )
			.slice( 0, limit );
	}

	items():SearchResult[]{
		const out = new Map<string,SearchResult>();
		this.#walk( this.#router.config, '', out );
		this.#dynamic( out );
		return [...out.values()];
	}

	#walk( routes:Routes, parentUrl:string, out:Map<string,SearchResult> ):void{
		for( const route of routes ){
			const path = route.path ?? '';
			if( path=='**' || path=='login' )
				continue;
			const segments = path.split('/').filter( s=>s.length );
			const url = parentUrl+(segments.length ? '/'+segments.join('/') : '') || '/';
			if( segments.some(s=>s.startsWith(':')) ){//not enumerable - except the ':collectionDisplay' list route, whose data.collections says what it stands for.
				const collections = route.data?.['collections'] as CollectionItem[]|undefined;
				if( segments.length==1 && collections ){
					for( const c of collections ){
						const item = typeof c=='string' ? { path: c } : c;
						const title = item.title ?? item.path.charAt(0).toUpperCase()+item.path.slice(1);
						const cUrl = `${parentUrl}/${item.path}`;
						if( !out.has(cUrl) )
							out.set( cUrl, { title, route: cUrl, icon: item.data?.icon, summary: item.data?.summary ?? RouteSearchProvider.parentSummary(parentUrl), source: this.name } );
					}
				}
				continue;
			}
			const title = typeof route.title=='string' ? route.title : undefined;
			if( title && !out.has(url) )
				out.set( url, { title, route: url, icon: route.data?.['icon'], summary: route.data?.['summary'], source: this.name } );
			if( route.children )
				this.#walk( route.children, url, out );
		}
	}

	//RouteStore keys are whatever the writer had in hand - 'gateways/gw1', '/apps', 'apps/gateways', bare 'users' - and a child
	//path may repeat the key's tail ('gateways/Debug' under 'apps/gateways').  Rebuild the absolute url from the candidates the
	//router actually matches rather than fixing every writer.
	#dynamic( out:Map<string,SearchResult> ):void{
		const topLevel = this.#router.config.map( r=>(r.path ?? '').split('/')[0] ).filter( p=>p.length && !p.startsWith(':') && p!='**' );
		for( const [key, children] of this.#routeStore.entries() ){
			const keySegments = key.split('/').filter( s=>s.length );
			for( const child of children ){
				if( !child?.title || !child.path )
					continue;
				const childSegments = child.path.split('/').filter( s=>s.length );
				const base = keySegments.length && childSegments[0]==keySegments[keySegments.length-1] ? keySegments.slice(0, -1) : keySegments;
				const direct = [ ...base, ...childSegments ];
				let segments:string[]|undefined = matchLiterals( this.#router.config, direct )>=0 ? direct : undefined;
				if( !segments ){//a bare key ('users'): try it under each top-level section.  The section and the key must both be spelled out by the route - a ':param' route accepts anything.
					const scored = topLevel.map( t=>[t, ...direct] ).map( c=>({ c, literals: matchLiterals(this.#router.config, c) }) ).filter( x=>x.literals>=1+base.length );
					if( scored.length )
						segments = scored.reduce( (best, x)=>x.literals>best.literals ? x : best ).c;
				}
				if( !segments )
					continue;
				const url = '/'+segments.map( s=>encodeURIComponent(s) ).join('/');
				if( !out.has(url) )
					out.set( url, { title: child.title, route: ['/'+segments[0], ...segments.slice(1)], icon: child.icon, summary: RouteSearchProvider.parentSummary('/'+segments.slice(0, -1).join('/')), source: this.name } );
			}
		}
	}
	private static parentSummary( parentUrl:string ):string|undefined{
		const segments = parentUrl.split('/').filter( s=>s.length );
		return segments.length ? segments.join(' / ') : undefined;
	}
}

import { inject, Injectable } from '@angular/core';
import { Router } from '@angular/router';
import { ISearchProvider, SearchResult } from 'jde-spa';
import { ENodeClass } from '../model/node';
import { Gateway, GATEWAY_SERVICE, GatewayService } from './gateway-service';

export type NodeSearchRow = { connection:{ target:string; name:string }; path:string; name:string; nodeClass:number; depth:number };

//OPC node names through the gateway's `search` query.  Inside a connection's node tree only that connection is searched;
//anywhere else every gateway is asked without an `opc`, which answers from the clients this session already holds - a search
//never opens an OPC session.  Hits render as name over `<connection>/<browse path>`.
@Injectable({ providedIn: 'root' })
export class NodeSearchProvider implements ISearchProvider{
	readonly name = 'nodes';
	readonly prefixes = [ 'node' ];
	#router = inject( Router );
	private gatewayService:GatewayService = inject( GATEWAY_SERVICE );

	static readonly columns = '{ connection{ target name } path name nodeClass depth }';
	static readonly currentConnection = /^\/gateways\/([^/?#]+)\/([^/?#]+)/;//app.routes.ts: gateways/:gateway/:connection/**

	async search( text:string, scope:string|undefined, limit:number ):Promise<SearchResult[]>{
		if( !text.length )
			return [];
		const hits:{ gateway:Gateway; row:NodeSearchRow }[] = [];
		const current = NodeSearchProvider.currentConnection.exec( this.#router.url );
		if( current ){
			const gateway = await this.gatewayService.gateway( decodeURIComponent(current[1]) );
			const rows = await gateway.queryArray<NodeSearchRow>( `search( opc:$opc, text:$text, limit:$limit )${NodeSearchProvider.columns}`, {opc: decodeURIComponent(current[2]), text, limit} );
			hits.push( ...rows.map( row=>({gateway, row}) ) );
		}
		else{
			const gateways = await this.gatewayService.gateways();
			const settled = await Promise.allSettled( gateways.map( g=>g.queryArray<NodeSearchRow>(`search( text:$text, limit:$limit )${NodeSearchProvider.columns}`, {text, limit}) ) );
			settled.forEach( (s,i)=>{
				if( s.status=='fulfilled' )
					hits.push( ...s.value.map( row=>({gateway: gateways[i], row}) ) );
				else
					console.warn( `search: gateway '${gateways[i].target}' failed.`, s.reason );//an unreachable gateway must not sink the others.
			} );
		}
		return hits.slice( 0, limit ).map( ({gateway, row})=>({
			title: row.name,
			route: [ '/gateways', gateway.target, row.connection.target, ...row.path.split('/') ],//array form: the router encodes each browse segment, NodeRoute decodes them back.
			summary: `${row.connection.name}/${row.path}`,
			icon: NodeSearchProvider.icon( row.nodeClass ),
			rank: row.name.toLowerCase().startsWith( text ) ? 0 : 1,
			source: this.name
		}) );
	}
	static icon( nodeClass:number ):string{
		switch( nodeClass ){
			case ENodeClass.Variable: return 'label';
			case ENodeClass.Method: return 'functions';
			default: return 'folder';
		}
	}
}

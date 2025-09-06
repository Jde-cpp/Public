import { inject, Inject, Injectable } from '@angular/core';
import { ActivatedRoute, Routes, UrlSegment } from "@angular/router";
import { DocItem, IRouteService } from "jde-spa";
import { RouteStore, subscribe } from "jde-framework";
import { GatewayService } from '../gateway.service';

@Injectable( {providedIn: 'root'} )
export class GatewayRouteService implements IRouteService{
	constructor( @Inject('GatewayService') private _gateway:GatewayService ){}
	async children():Promise<Routes>{
		throw new Error("Not implemented");
	}

	async docItems( urlSegments:UrlSegment[] ):Promise<DocItem[]>{
		let y = [];
		let instances = await this._gateway.instances();
		for( const s of instances )
			y.push( new DocItem({path: s.host, title: s.host}) );

		this.routeStore.setSiblings( urlSegments, y );
		return y;
	}

	routeStore = inject( RouteStore );
}

@Injectable( {providedIn: 'root'} )
export class GatewayCnnctnRouteService implements IRouteService{
	constructor( @Inject('GatewayService') private _gateway:GatewayService, private _route: ActivatedRoute ){
	}
	async children():Promise<Routes>{
		throw new Error("Not implemented");
	}

	async docItems( urlSegments:UrlSegment[] ):Promise<DocItem[]>{
		let y = [];
		let route = this._route.snapshot.children[0];
		let host = route.paramMap.get("host");
		let gateway = await this._gateway.instance( host );
		let connections = await gateway.queryArray<any>( `serverConnections{ name target }`,  );
		for( const c of connections )
			y.push( new DocItem({path: c.target, title: c.name}) );

		this.routeStore.setSiblings( route.url, y );
		return y;
	}

	routeStore = inject( RouteStore );
}
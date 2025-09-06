import { Injectable } from '@angular/core';
import { ActivatedRoute, Routes, UrlSegment } from "@angular/router";
import { DocItem } from '../component-sidenav/component-sidenav';


export interface IRouteService{
	children( x:UrlSegment[] ):Promise<Routes>;
	docItems( x:UrlSegment[] ):Promise<DocItem[]>;
}

@Injectable()
export class RouteService implements IRouteService{
	constructor( protected route: ActivatedRoute )
	{}
	children( urlSegments:UrlSegment[] ):Promise<Routes>{
		let config = this.route.parent?.routeConfig ?? this.route.routeConfig;
		return Promise.resolve( config.children );
	}
	async docItems( urlSegments:UrlSegment[] ):Promise<DocItem[]>{
		const children = await this.children(urlSegments);
		let items = [];
		for( let child of children ){
			var docItem = <DocItem>{...child.data, path:child.path, title: <string>child.title };
			// if( !docItem.path )
			// 	docItem.path = child.path;
			if( child.path.length )
				items.push( docItem );
		}
		return items;
	}
}
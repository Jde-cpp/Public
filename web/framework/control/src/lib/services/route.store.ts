import { HostListener, Injectable } from "@angular/core";
import { UrlSegment } from "@angular/router";
import { RouteItem } from "jde-spa";
import { load } from "protobufjs";

@Injectable({ providedIn: 'root' })
export class RouteStore{
	getChildren( url:string | UrlSegment[] ):RouteItem[]{
		if( Array.isArray(url) )
			url = url.map( x=>x.path ).join("/");
		return this.#children.get( url ) ?? this.loadChildren( url );
	}

	setChildren( url:string | UrlSegment[], children:RouteItem[] ){
		if( Array.isArray(url) )
			url = url.map( x=>x.path ).join("/");
		this.#children.set( url, children.map(s=>{return new RouteItem({title:s.title, path:s.path})}) );
		localStorage.setItem( url, JSON.stringify(this.#children.get(url)) );
	}

	private loadChildren( key:string ):RouteItem[]{
		let storage = localStorage.getItem( key );
		let children:RouteItem[] = [];
		if( storage ){
			children = JSON.parse(storage).map( (s:any)=>new RouteItem(s) );
			this.#children.set( key, children );
		}
		return children;
	}

	#children:Map<string,RouteItem[]> = new Map<string,RouteItem[]>();//parent url, children relative to parent
}
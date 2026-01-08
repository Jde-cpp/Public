import { HostListener, Injectable } from "@angular/core";
import { UrlSegment } from "@angular/router";
import { DocItem } from "jde-spa";
import { load } from "protobufjs";

@Injectable({ providedIn: 'root' })
export class RouteStore{
	getChildren( url:string | UrlSegment[] ):DocItem[]{
		if( Array.isArray(url) )
			url = url.map( x=>x.path ).join("/");
		return this.#children.get( url ) ?? this.loadChildren( url );
	}

	setChildren( url:string | UrlSegment[], children:DocItem[] ){
		if( Array.isArray(url) )
			url = url.map( x=>x.path ).join("/");
		this.#children.set( url, children.map(s=>{return new DocItem({title:s.title, path:s.path})}) );
		localStorage.setItem( url, JSON.stringify(this.#children.get(url)) );
	}

	private loadChildren( key:string ):DocItem[]{
		let storage = localStorage.getItem( key );
		let children:DocItem[] = [];
		if( storage ){
			children = JSON.parse(storage).map( (s:any)=>new DocItem(s) );
			this.#children.set( key, children );
		}
		return children;
	}

	#children:Map<string,DocItem[]> = new Map<string,DocItem[]>();//parent url, children relative to parent
}
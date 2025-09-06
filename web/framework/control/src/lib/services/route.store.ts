import { HostListener, Injectable } from "@angular/core";
import { UrlSegment } from "@angular/router";
import { DocItem } from "jde-spa";
import { load } from "protobufjs";

@Injectable({ providedIn: 'root' })
export class RouteStore{
	getSiblings( url:string | UrlSegment[] ):DocItem[]{
		if( Array.isArray(url) )
			url = url.map( x=>x.path ).join("/");
		return this.#siblings.get( url ) ?? this.loadSiblings( url );
	}

	setSiblings( url:string | UrlSegment[], siblings:DocItem[] ){
		if( Array.isArray(url) )
			url = url.map( x=>x.path ).join("/");
		this.#siblings.set( url, siblings.map(s=>{return new DocItem({title:s.title, path:s.path})}) );
		localStorage.setItem( url, JSON.stringify(this.#siblings.get(url)) );
	}

	private loadSiblings( key:string ):DocItem[]{
		let storage = localStorage.getItem( key );
		let siblings:DocItem[] = [];
		if( storage ){
			siblings = JSON.parse(storage).map( (s:any)=>new DocItem(s) );
			this.#siblings.set( key, siblings );
		}
		return siblings;
	}

	/*TODO move to somewhere that works.
	@HostListener( 'window:beforeunload', ['$event'] )
  beforeUnloadHandler(event: BeforeUnloadEvent) {
		debugger;
    for (const key of this.#siblings.keys()) {
			localStorage.setItem( key, JSON.stringify(this.#siblings.get(key)) );
		}
  }
*/
	#siblings:Map<string,DocItem[]> = new Map<string,DocItem[]>();//parent url, children relative to parent
}
import {Params} from '@angular/router';

export class RouteItem{
	constructor( args?:Partial<RouteItem>){
		if( args )
			Object.assign( this, args );
	}
	cardClass?: string;
	externalRedirect?: string;
	icon?: string;
	parent?:RouteItem;
	get path(){ return this._path; } set path(x){ this._path=x; } private _path!: string; //routerLink - access/groups or relative
	get queryParams(){ return this._queryParams; } set queryParams(x){ this._queryParams=x; } private _queryParams!: Params;
	siblings?:RouteItem[]; //includes this.
	summary?: string;
	get title(){ return this._title; } set title(x){ this._title=x; } private _title!: string;
	get track(){ return this.queryParams ? this.path+JSON.stringify(this.queryParams) : this.path; }
}

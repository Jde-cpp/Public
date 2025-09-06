import { Injectable } from '@angular/core';
import { DocItem, IRouteService } from "jde-spa";
import { Routes, UrlSegment } from "@angular/router";

@Injectable( {providedIn: 'root'} )
export class OpcNodeRouteService implements IRouteService{
	async children():Promise<Routes>{
		throw new Error("Not implemented");
	}
	async docItems( urlSegments:UrlSegment[] ):Promise<DocItem[]>{
		debugger;
		return Promise.resolve( [] );
	}
}
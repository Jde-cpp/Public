import { DocItem } from "jde-spa";
import * as types from './types';
import {NodeId} from './NodeId';
import { ActivatedRouteSnapshot, Params } from "@angular/router";
import { OpcStore } from "../services/opc-store";
import { OpcObject, UaNode } from "./Node";
import { Inject } from "@angular/core";
import { GatewayTarget } from "../services/gateway.service";
import { Browse, Ns } from "./types";

export class NodeRoute extends DocItem{
	constructor( activatedRoute:ActivatedRouteSnapshot, @Inject("OpcStore") opcStore:OpcStore ){
		super();
		let paramsRoute = activatedRoute.pathFromRoot.find( (r)=>r.paramMap.get("gateway") );
		this.gatewayTarget = paramsRoute?.paramMap.get("gateway");
		this.cnnctnTarget = paramsRoute?.paramMap.get("connection");
		this.route = activatedRoute.pathFromRoot[activatedRoute.pathFromRoot.length-1];
		this.node = this.browsePath.length ? opcStore.findNodeId( this.gatewayTarget, this.cnnctnTarget, this.browsePath ) : OpcObject.rootNode;
	}
	browse(defaultNs:Ns):Browse{
		return types.toBrowse( this.route.url[this.route.url.length-1].path, defaultNs );
	}

	route: ActivatedRouteSnapshot;
	override get path(): string{
		return this.route.url.map(seg=>seg.path).join("/");
	}
	override get title(): string{ return this.route.title; }
	override get queryParams():Params{ return this.route.queryParams; }
	collectionName=null;
	node: UaNode;
	get nodeId():NodeId{ return this.node; }
	get browsePath():string{
		return this.route.url.map(seg=>seg.path).join("/");
	}
	get profileKey():string{ return this.nodeId?.toString(); }
	children: DocItem[];
	cnnctnTarget:string;
	gatewayTarget:GatewayTarget;
}

import { NgModule } from '@angular/core';
import {Routes, ROUTES, RouterModule} from '@angular/router';

import{ DetailResolver, QLListResolver, QLListRouteService, HomeRouteService, AppResolver } from 'jde-framework';
import { AccessService, AuthGuard, Group, groupTableSettings, Role, roleTableSettings, User, userTableSettings } from 'jde-access';
import{ ClientResolver, GatewayRouteService, gatewayTableSettings, GatewayCnnctnRouteService,GatewayService, NodeResolver, OpcNodeRouteService, GatewayResolver } from 'jde-opc';

const accessProvider = { provide: 'IGraphQL', useExisting: AccessService };//route-scoped token, but aliases the single providedIn:'root' instance instead of constructing a per-route one
const gatewayProvider = { provide: 'IGraphQL', useExisting: GatewayService };//route-scoped token, but aliases the single providedIn:'root' instance instead of constructing a per-route one
const qlListProvider = { provide: 'IRouteService', useClass: QLListRouteService };
const opcNodeRouteProvider = { provide: 'IRouteService', useExisting: OpcNodeRouteService };//NodeChildren injects the class token, so the route binding must alias that instance rather than build a second one

//pages are loadComponent, not component:  an eager reference drags the page and its Material deps into the initial bundle, which blew the 2mb size budget
const sidenav = ()=>import('jde-spa').then( m=>m.ComponentSidenav );
const cards = ()=>import('jde-framework').then( m=>m.Cards );

export const routes: Routes = [
	{ path: '', title: "Home", loadComponent: cards, data: {summary: "Welcome" },
		canActivate: [AuthGuard],
		providers: [  {provide: 'IRouteService', useClass: HomeRouteService} ]},
	{ path: 'login', loadComponent: ()=>import('jde-framework').then( m=>m.LoginPageComponent ), data: {name: "Login", summary: "Login to Site"} },
	{ path: 'gateways', title: "Gateways", canActivate: [AuthGuard], loadComponent: cards,
		providers: [{provide: 'IRouteService', useClass: GatewayRouteService}],
		data: {summary: "Available Gateways",}
	},
	{ path: 'gateways/:gateway', title: ":gateway", canActivate: [AuthGuard], loadComponent: cards,
		providers: [{provide: 'IRouteService', useClass: GatewayCnnctnRouteService}],
		data: {summary: "Available Connections",}
	},
	{
		path: 'gateways/:gateway/:connection', loadComponent: sidenav, canActivate: [AuthGuard],
		children :[
			{
				path: '**',
				loadComponent: ()=>import('jde-opc').then( m=>m.NodeDetail ),
				providers: [ NodeResolver, opcNodeRouteProvider ],
				canActivate: [AuthGuard],
				data: { summary: "Opc Gateway Detail", collectionName: "serverConnections" },
				resolve: { pageData: NodeResolver },
				runGuardsAndResolvers: "pathParamsOrQueryParamsChange"
			}
		]
	},
	{ path: 'access', title: "Access", loadComponent: cards, providers: [qlListProvider], canActivate: [AuthGuard], data: {
		summary: "Configure User Access"
	} },
	{ path: 'access', loadComponent: sidenav, canActivate: [AuthGuard], providers: [qlListProvider],
			children :[
				{ path: 'users/:target',
					loadComponent: ()=>import('jde-access').then( m=>m.UserDetail ),
					providers: [ DetailResolver<User>, accessProvider ],
					resolve: { pageData: DetailResolver<User> },
					canActivate: [AuthGuard],
					runGuardsAndResolvers: "paramsChange"
				},
				{ path: 'groups/:target',
					loadComponent: ()=>import('jde-access').then( m=>m.GroupDetail ),
					providers: [ DetailResolver<Group>, accessProvider ],
					resolve: { pageData: DetailResolver<Group> },
					canActivate: [AuthGuard],
					runGuardsAndResolvers: "paramsChange",
				},
				{ path: 'roles/:target',
					loadComponent: ()=>import('jde-access').then( m=>m.RoleDetail ),
					providers: [ DetailResolver<Role>, accessProvider ],
					data: { summary: "Role Detail" },
					resolve: { pageData: DetailResolver<Role> },
					canActivate: [AuthGuard],
					runGuardsAndResolvers: "paramsChange"
				},
				{ path: ':collectionDisplay',
					loadComponent: ()=>import('jde-framework').then( m=>m.QLList ),
					runGuardsAndResolvers: "paramsChange",
					providers: [ QLListResolver, accessProvider ],
					resolve: { data: QLListResolver },
					canActivate: [AuthGuard],
					data: { collections: [
						{ path:"users", data:{tableSettings: userTableSettings} },
						{ path:"groups", data:{tableSettings: groupTableSettings} },
						{ path: "roles", data:{tableSettings: roleTableSettings} },
						{ path:"resources", data:{canPurge:false} }
					]}
				},
			]
	},
	{
		path: 'apps',
		title: "Applications",
		canActivate: [AuthGuard],
		loadComponent: ()=>import('jde-framework').then( m=>m.Apps ),
		providers: [ AppResolver, accessProvider ],
		resolve: { connections: AppResolver },
	},
	{
		path: 'apps/gateways/:instance', title: ":instance", loadComponent: sidenav, canActivate: [AuthGuard],
		children :[
			{
				path: '',
				loadComponent: ()=>import('jde-opc').then( m=>m.GatewayDetail ),
				providers:[ GatewayResolver, gatewayProvider],
				resolve: {data: GatewayResolver},
				canActivate: [AuthGuard],
				data: { tableSettings: gatewayTableSettings }
			},
			{
				path: ':connection',
				loadComponent: ()=>import('jde-opc').then( m=>m.ClientDetail ),
				providers: [ ClientResolver, gatewayProvider ],
				canActivate: [AuthGuard],
				data: { summary: "Opc Connection", collectionName: "serverConnections" },
				resolve: { pageData: ClientResolver }
			}
		]
	}
];
function setRoutes(){
	return routes;
}

@NgModule( { imports: [RouterModule.forRoot([])], exports: [RouterModule],
	providers: [
		{ provide: ROUTES, useFactory: setRoutes, multi: true }]//AuthGuard is providedIn:'root'; listing it here built a second one for this module's injector
})
export class AppRoutingModule
{}

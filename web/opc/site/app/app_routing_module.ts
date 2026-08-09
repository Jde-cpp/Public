import { NgModule } from '@angular/core';
import {Routes, ROUTES, RouterModule} from '@angular/router';
import { ComponentSidenav } from 'jde-spa';

import{ DetailResolver, Cards, LoginPageComponent, QLList, QLListResolver, QLListRouteService, HomeRouteService, Apps, AppResolver } from 'jde-framework';
import { AccessService, AuthGuard, Group, GroupDetail, groupTableSettings, Role, RoleDetail, roleTableSettings, User, UserDetail, userTableSettings } from 'jde-access';
import{ ClientResolver, GatewayDetail, GatewayRouteService, gatewayTableSettings, GatewayCnnctnRouteService,GatewayService, NodeDetail, NodeResolver, OpcNodeRouteService, GatewayResolver, ClientDetail } from 'jde-opc';

const accessProvider = { provide: 'IGraphQL', useExisting: AccessService };//route-scoped token, but aliases the single providedIn:'root' instance instead of constructing a per-route one
const gatewayProvider = { provide: 'IGraphQL', useExisting: GatewayService };//route-scoped token, but aliases the single providedIn:'root' instance instead of constructing a per-route one
const qlListProvider = { provide: 'IRouteService', useClass: QLListRouteService };
const opcNodeRouteProvider = { provide: 'IRouteService', useExisting: OpcNodeRouteService };//NodeChildren injects the class token, so the route binding must alias that instance rather than build a second one

export const routes: Routes = [
	{ path: '', title: "Home", component: Cards, data: {summary: "Welcome" },
		canActivate: [AuthGuard],
		providers: [  {provide: 'IRouteService', useClass: HomeRouteService} ]},
	{ path: 'login', component: LoginPageComponent, data: {name: "Login", summary: "Login to Site"} },
	{ path: 'gateways', title: "Gateways", canActivate: [AuthGuard], component: Cards,
		providers: [{provide: 'IRouteService', useClass: GatewayRouteService}],
		data: {summary: "Available Gateways",}
	},
	{ path: 'gateways/:gateway', title: ":gateway", canActivate: [AuthGuard], component: Cards,
		providers: [{provide: 'IRouteService', useClass: GatewayCnnctnRouteService}],
		data: {summary: "Available Connections",}
	},
	{
		path: 'gateways/:gateway/:connection', component: ComponentSidenav, canActivate: [AuthGuard],
		children :[
			{
				path: '**',
				component: NodeDetail,
				providers: [ NodeResolver, opcNodeRouteProvider ],
				canActivate: [AuthGuard],
				data: { summary: "Opc Gateway Detail", collectionName: "serverConnections" },
				resolve: { pageData: NodeResolver },
				runGuardsAndResolvers: "pathParamsOrQueryParamsChange"
			}
		]
	},
	{ path: 'access', title: "Access", component: Cards, providers: [qlListProvider], canActivate: [AuthGuard], data: {
		summary: "Configure User Access"
	} },
	{ path: 'access', component: ComponentSidenav, canActivate: [AuthGuard], providers: [qlListProvider],
			children :[
				{ path: 'users/:target',
					component: UserDetail,
					providers: [ DetailResolver<User>, accessProvider ],
					resolve: { pageData: DetailResolver<User> },
					canActivate: [AuthGuard],
					runGuardsAndResolvers: "paramsChange"
				},
				{ path: 'groups/:target',
					component: GroupDetail,
					providers: [ DetailResolver<Group>, accessProvider ],
					resolve: { pageData: DetailResolver<Group> },
					canActivate: [AuthGuard],
					runGuardsAndResolvers: "paramsChange",
				},
				{ path: 'roles/:target',
					component: RoleDetail,
					providers: [ DetailResolver<Role>, accessProvider ],
					data: { summary: "Role Detail" },
					resolve: { pageData: DetailResolver<Role> },
					canActivate: [AuthGuard],
					runGuardsAndResolvers: "paramsChange"
				},
				{ path: ':collectionDisplay',
					component: QLList,
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
		component: Apps,
		providers: [ AppResolver, accessProvider ],
		resolve: { connections: AppResolver },
	},
	{
		path: 'apps/gateways/:instance', title: ":instance", component: ComponentSidenav, canActivate: [AuthGuard],
		children :[
			{
				path: '',
				component: GatewayDetail,
				providers:[ GatewayResolver, gatewayProvider],
				resolve: {data: GatewayResolver},
				canActivate: [AuthGuard],
				data: { tableSettings: gatewayTableSettings }
			},
			{
				path: ':connection',
				component: ClientDetail,
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
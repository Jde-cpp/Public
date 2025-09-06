import { NgModule } from '@angular/core';
import {Routes, ROUTES, RouterModule} from '@angular/router';

import { ComponentCategoryList, ComponentSidenav } from 'jde-spa';
import{ Applications, DetailResolver, Home, LoginPageComponent, QLList, QLListResolver } from 'jde-framework';
import{ AccessService, Group, Role, RoleDetail, UserDetail, GroupDetail, User } from 'jde-access';

const accessProvider = { provide: 'IGraphQL', useClass: AccessService };
export const routes: Routes = [
	{ path: '', title: "Home", component: Home, data: {summary: "Welcome" } },
	{ path: 'login', title: "Login", component: LoginPageComponent, data: {summary: "Login to Site"} },
	{ path: 'access', title: "Access", component: ComponentSidenav, data: { summary: "Configure Access"},
			children :[
			{ path: '', title: "Access", component: ComponentCategoryList },
			{
				path: 'users/:target',
				component: UserDetail,
				providers: [ DetailResolver<User>, accessProvider ],
				resolve: { pageData: DetailResolver<User> }
			},
			{
				path: 'groups/:target',
				component: GroupDetail,
				providers: [ DetailResolver<Group>, accessProvider ],
				resolve: { pageData: DetailResolver<Group> }
			},
			{
				path: 'roles/:target',
				component: RoleDetail,
				providers: [ DetailResolver<Role>, accessProvider ],
				data: { summary: "Role Detail" },
				resolve: { pageData: DetailResolver<Role> }
			},
			{
				path: ':collectionDisplay',
				component: QLList,
				runGuardsAndResolvers: "always",
				providers: [ QLListResolver, accessProvider ],
				resolve: { data: QLListResolver },
				data: { collections: [
					"users",
					{id:"groups", collectionName: "groupings"},
					"roles",
					{id:"resources", canPurge:false}] }
			},
		]
	},
	{
		path: 'settings',
		title: "Settings",
		component: ComponentSidenav,
		data: { summary: "Settings Main Menu" },
		children :[
			{ path: '', title: "Settings", component: ComponentCategoryList, data: { summary: "Settings Sub outlet" } },
			{ path: 'applications', title:"Applications", component: Applications, data: { summary: "View Applications" } },
		]
	}
];
function setRoutes(){
	return routes;
}

@NgModule( { imports: [RouterModule.forRoot([])], exports: [RouterModule],
	providers: [{
      provide: ROUTES,
      useFactory: setRoutes,
			multi: true
	}]})
 export class AppRoutingModule {
 }

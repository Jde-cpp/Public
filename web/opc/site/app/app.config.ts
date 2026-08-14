import { provideHttpClient } from "@angular/common/http";
import { ApplicationConfig } from '@angular/core';
import { provideRouter } from '@angular/router';
import { AppService, AuthStore, ProfileService } from 'jde-framework'
import { GatewayService, OpcAuthService, OpcStore} from 'jde-opc';
import {EnvironmentService} from './services/environment.service';
import { routes } from './app_routing_module';
import { AccessService } from "jde-access";

export const appConfig: ApplicationConfig = {
  providers: [
		provideHttpClient(),
		provideRouter(routes),
		{provide: "AccessService", useExisting: AccessService},
		{provide: 'AppService', useExisting: AppService},
		{provide: 'IAuth', useClass: OpcAuthService},
		{provide: "AuthStore", useClass: AuthStore},
		{provide: 'IEnvironment', useClass: EnvironmentService},
		{provide: 'GatewayService', useExisting: GatewayService},//useExisting, not useClass:  useClass is a construction recipe, so each token would build its own GatewayService (and its own sockets/queries)
		{provide: 'OpcStore', useExisting: OpcStore},//string-token writers (GatewayService, NodeResolver, NodeRoute) and class-token readers (ClientResolver) must share one store
		{provide: 'IProfileService', useExisting: ProfileService},//ProfileStore (jde-spa) persists via this token; jde-spa can't import the framework implementation
		//OpcNodeRouteService/AuthGuard need no string token - every consumer injects the class, which providedIn:'root' already supplies
	]
};
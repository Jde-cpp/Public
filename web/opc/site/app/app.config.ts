import { provideHttpClient } from "@angular/common/http";
import { ApplicationConfig } from '@angular/core';
import { provideRouter } from '@angular/router';
import { AppService, AuthStore, DefaultErrorService } from 'jde-framework'
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
		{provide: 'IErrorService', useClass: DefaultErrorService},
		{provide: 'GatewayService', useExisting: GatewayService},//useExisting, not useClass:  useClass is a construction recipe, so each token would build its own GatewayService (and its own sockets/queries)
		{provide: 'OpcStore', useExisting: OpcStore},//string-token writers (GatewayService, NodeResolver, NodeRoute) and class-token readers (ClientResolver) must share one store
		//ProfileStore/OpcNodeRouteService/AuthGuard need no string token - every consumer injects the class, which providedIn:'root' already supplies
	]
};
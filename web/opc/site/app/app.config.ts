import { provideHttpClient } from "@angular/common/http";
import { ApplicationConfig } from '@angular/core';
import { provideAnimationsAsync } from '@angular/platform-browser/animations/async';
import { provideRouter } from '@angular/router';
import { AppService, AuthStore, DefaultErrorService,LocalProfileStore } from 'jde-framework'
import { GatewayService, OpcAuthService, OpcNodeRouteService, OpcStore} from 'jde-opc';
import {EnvironmentService} from './services/environment.service';
import { routes } from './app_routing_module';
import { AuthGuard, AccessService } from "jde-access";

export const appConfig: ApplicationConfig = {
  providers: [provideAnimationsAsync(),
		provideHttpClient(),
		provideRouter(routes),
		{provide: "AccessService", useClass: AccessService},
		{provide: 'AppService', useClass: AppService},
		{provide: 'IAuth', useClass: OpcAuthService},
		{provide: "AuthGuard", useClass: AuthGuard},
		{provide: "AuthStore", useClass: AuthStore},
		{provide: 'IEnvironment', useClass: EnvironmentService},
		{provide: 'IErrorService', useClass: DefaultErrorService},
		{provide: 'GatewayService', useClass: GatewayService},
		{provide: 'OpcStore', useClass: OpcStore},
		{provide: 'IProfileStore', useClass: LocalProfileStore},
		{provide: "OpcNodeRouteService", useClass: OpcNodeRouteService},
	]
};
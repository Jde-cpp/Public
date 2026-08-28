import { provideHttpClient } from "@angular/common/http";
import { ApplicationConfig } from '@angular/core';
import { MAT_TABS_CONFIG } from '@angular/material/tabs';
import { MAT_NATIVE_DATE_FORMATS, MatDateFormats, provideNativeDateAdapter } from '@angular/material/core';
import { provideRouter } from '@angular/router';
import { AppService, AuthStore, ProfileService } from 'jde-framework'
import { GatewayService, NodeSearchProvider, OpcAuthService, OpcStore} from 'jde-opc';
import { RouteSearchProvider, SEARCH_PROVIDERS } from 'jde-spa';
import {EnvironmentService} from './services/environment-service';
import { routes } from './app.routes';
import { AccessSearchProvider, AccessService } from "jde-access";

//2-digit rather than the native numeric:  it zero-pads the datepicker input ("08/27/2026", not "8/27/2026"), so a column
//of dates is one width and lines up when right-aligned.  These are Intl.DateTimeFormat OPTIONS, not a pattern string, so
//the field order stays the locale's - 08/27/2026 in en-US, 27/08/2026 in en-GB, 27.08.2026 in de-DE.  Only dateInput is
//overridden;  the a11y labels and the month/year header keep the native spelling.
const dateFormats:MatDateFormats = {
	...MAT_NATIVE_DATE_FORMATS,
	display: {...MAT_NATIVE_DATE_FORMATS.display, dateInput: {year: 'numeric', month: '2-digit', day: '2-digit'}}
};

export const appConfig: ApplicationConfig = {
  providers: [
		provideHttpClient(),
		provideRouter(routes),
		//every datepicker in the app takes its DateAdapter and formats from here - the components used to provide their own,
		//which is one adapter instance per component and as many places to edit.  One provider is also the single point to
		//swap the native adapter for a locale-aware one:  NativeDateAdapter.parse ignores the format and calls Date.parse, so
		//it reads en-US/ISO input and nothing else.  MAT_DATE_LOCALE is deliberately NOT provided - it already defaults to
		//LOCALE_ID, which is the one knob to set when the app is localised.
		provideNativeDateAdapter( dateFormats ),
		//0ms kills both tab animations at once:  MatTabGroup feeds animationDuration to --mat-tab-body-animation-duration (the body slide) and --mat-tab-header-animation-duration (the ink bar), and flags the group noopable.  dynamicHeight (the wrapper-height transition) is off by default and must stay off in the templates - an attribute there overrides this.
		{provide: MAT_TABS_CONFIG, useValue: {animationDuration: '0ms'}},
		{provide: "AccessService", useExisting: AccessService},
		{provide: 'AppService', useExisting: AppService},
		{provide: 'IAuth', useClass: OpcAuthService},
		{provide: "AuthStore", useClass: AuthStore},
		{provide: 'IEnvironment', useClass: EnvironmentService},
		{provide: 'GatewayService', useExisting: GatewayService},//useExisting, not useClass:  useClass is a construction recipe, so each token would build its own GatewayService (and its own sockets/queries)
		{provide: 'OpcStore', useExisting: OpcStore},//string-token writers (GatewayService, NodeResolver, NodeRoute) and class-token readers (ClientResolver) must share one store
		{provide: 'IProfileService', useExisting: ProfileService},//ProfileStore (jde-spa) persists via this token; jde-spa can't import the framework implementation
		//the navbar search (jde-spa) fans out through this multi token, same reason;  registration order is result precedence.
		{provide: SEARCH_PROVIDERS, useExisting: RouteSearchProvider, multi: true},
		{provide: SEARCH_PROVIDERS, useExisting: AccessSearchProvider, multi: true},
		{provide: SEARCH_PROVIDERS, useExisting: NodeSearchProvider, multi: true},
		//OpcNodeRouteService/AuthGuard need no string token - every consumer injects the class, which providedIn:'root' already supplies
	]
};
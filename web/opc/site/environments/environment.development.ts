//swapped in for environment.ts by the build's `development` configuration (angular.json fileReplacements), so
//`ng serve` and `ng build --configuration development` get these values and a production build never sees them.
//Keep the key set identical to environment.ts:  EnvironmentService.get() looks keys up by name, so one spelled
//only here reads back undefined in production instead of failing the build.
//Dev-only overrides belong here - e.g. googleCredential, a signed-in id token that lets LoginPage skip
//the interactive Google prompt (Google refuses that prompt in a CDP-driven browser).
export const environment = {
	defaultNS: 0,
	applicationServer: {port:1967, host:"localhost"},
	production: false
};

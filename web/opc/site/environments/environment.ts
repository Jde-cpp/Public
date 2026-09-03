import { ETransport } from 'jde-framework';

//httpTransport picks ws/http (Unsecure), wss/https (Secure), or ws/http with an https escape hatch for the calls that ask
//for it (Hybrid).  AppService reads it by name, so the key has to exist in BOTH files or production reads back undefined.
export const environment = {
	defaultNS: 0,
	httpTransport: ETransport.Unsecure,
	applicationServer: {port:1967, host:"localhost"},
	production: true
};
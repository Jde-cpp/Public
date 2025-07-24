import { ActivatedRoute, ActivatedRouteSnapshot, Resolve, Router, RouterStateSnapshot } from '@angular/router';
import { inject, Inject, Injectable } from '@angular/core';
import { IErrorService, IProfile, Settings, TableSchema } from 'jde-framework';
import { Role, RoleNK } from '../model/Role';
import { AccessService } from '../services/access.service';

export class IRoleData{
	profile:Settings<UserSettings>;
	role:Role;
	schema: TableSchema;
};
@Injectable()
export class RoleResolver implements Resolve<IRoleData> {
	constructor( private route: ActivatedRoute, private router:Router, @Inject('IProfile') private profileService: IProfile, @Inject('IErrorService') private snackbar: IErrorService ){}

	async load(target:RoleNK):Promise<IRoleData>{
		const profile = new Settings<UserSettings>( UserSettings, `role-properties`, this.profileService );
		await profile.loadedPromise;
		const schema = await this.#ql.schemaWithEnums( "roles" );

		let ql = `role( target: "${target}" ){ id target name created updated ${profile.value.showDeleted ? "deleted" : ""} description permissionRights{id allowed denied resource{id}} roles{id} }`;
		try{
			const role = await this.#ql.querySingle(ql);
			return { profile: profile, role: new Role(role), schema: schema };
		}catch( e ){
			this.snackbar.error( `Role not found:  '${target}'` );
			this.router.navigate( ['..'], { relativeTo: this.route } );
			return null;
		}
	}
	resolve(route: ActivatedRouteSnapshot, state: RouterStateSnapshot):Promise<IRoleData>{
		return this.load( route.paramMap.get("target") );
	}

	#ql = inject( AccessService );
}
export class UserSettings{
	assign( value:UserSettings ){ this.tabIndex = value.tabIndex;  }
	showDeleted:boolean = false;
	tabIndex:number=0;
}
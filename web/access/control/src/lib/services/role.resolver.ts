import { ActivatedRoute, ActivatedRouteSnapshot, Resolve, Router, RouterStateSnapshot } from '@angular/router';
import { inject, Inject, Injectable } from '@angular/core';
import { IErrorService, LocalProfileStore, TableSchema } from 'jde-framework';
import { Role, RoleNK } from '../model/Role';
import { AccessService } from '../services/access.service';

export class IRoleData{
	role:Role;
	schema: TableSchema;
};
@Injectable()
export class RoleResolver implements Resolve<IRoleData> {
	constructor( private route: ActivatedRoute, private router:Router, @Inject('IErrorService') private snackbar: IErrorService ){}

	async load(target:RoleNK):Promise<IRoleData>{
		const schema = await this.#ql.schemaWithEnums( "roles", (m)=>console.log(m) );

		let ql = `role( target: "${target}" ){ id target name created updated ${LocalProfileStore.showDeleted("roles") ? "deleted" : ""} description permissionRights{id allowed denied resource{id}} roles{id} }`;
		try{
			const role = await this.#ql.querySingle(ql);
			return { role: new Role(role), schema: schema };
		}catch( e ){
			this.snackbar.error( `Role not found:  '${target}'`, (m)=>console.error(m) );
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
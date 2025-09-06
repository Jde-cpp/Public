import {Injectable} from '@angular/core';
import {Title} from '@angular/platform-browser';

/**
 * Service responsible for setting the title that appears above the components and guide pages.
 */
@Injectable({providedIn: 'root'})
export class ComponentPageTitle {
  _title = '';
  //_originalTitle = 'Angular Material UI component library';

  get title(): string {
    return this._title;
  }

  set title(title: string) {
    this._title = title;
    if (title !== '') {
      //title = `${title} | Angular Material`;
      title = title; //
    } else {
      title = 'Jde';//this._originalTitle;
    }
    this.bodyTitle.setTitle(title);
  }
	set detail( x:string ){//
		//const main = this.title?.includes("|") ? this.title.substring( this.title.lastIndexOf('|') ) : this.title;
		this.title = `${x}`;
 	}
  constructor(private bodyTitle: Title) {}
}

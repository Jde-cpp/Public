import {Directive, ElementRef, OnDestroy, inject } from '@angular/core';
import {NavigationFocusService} from './navigation-focus-service';

let uid = 0;
@Directive({
  selector: '[focusOnNavigation]',
  //bindings, not static attributes:  @HostBinding bound the tabindex PROPERTY and the outline style, and a static
  //`tabindex` in here could be overridden by one on the host element instead of winning (review3 C9).
  host: {
    '[tabindex]': '"-1"',
    '[style.outline]': '"none"'
  }
})
export class NavigationFocus implements OnDestroy {
  private el:ElementRef = inject( ElementRef );
  private navigationFocusService:NavigationFocusService = inject( NavigationFocusService );
  constructor() {
    if (!this.el.nativeElement.id) {
      this.el.nativeElement.id = `skip-link-target-${uid++}`;
    }
    this.navigationFocusService.requestFocusOnNavigation(this.el.nativeElement);
    this.navigationFocusService.requestSkipLinkFocus(this.el.nativeElement);
  }

  ngOnDestroy() {
    this.navigationFocusService.relinquishFocusOnNavigation(this.el.nativeElement);
    this.navigationFocusService.relinquishSkipLinkFocus(this.el.nativeElement);
  }
}

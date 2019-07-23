import { Component, ViewChild, OnInit, OnDestroy, AfterViewInit, ViewEncapsulation  } from '@angular/core';
import { ControllerService } from '@app/services/controller.service';
import { Eventtypes } from '@app/enum/eventtypes.enum';
import { OverlayRef, Overlay, GlobalPositionStrategy } from '@angular/cdk/overlay';
import { Subscription } from 'rxjs';
import { Portal, TemplatePortalDirective } from '@angular/cdk/portal';
import { HttpClient } from '@angular/common/http';
import { HelpLinkMap } from 'assets/generated/docs/help/linkMap';
import { Router, NavigationEnd } from '@angular/router';
import { filter } from 'rxjs/operators';
import { UrlMap } from 'assets/generated/docs/help/urlMap';

enum HistoryEvent {
  'forward'= 'forward',
  'backward'= 'backward',
  'new'= 'new'
}


@Component({
  selector: 'app-helpoverlay',
  templateUrl: './helpoverlay.component.html',
  styleUrls: ['./helpoverlay.component.scss', './markdown.scss'],
  encapsulation: ViewEncapsulation.None
})
export class HelpoverlayComponent implements OnInit, OnDestroy, AfterViewInit {
  subscriptions: Subscription[] = [];
  urlContentSubscription: Subscription;
  helpPortal: Portal<any>;
  @ViewChild('HelpShell') helpShell: TemplatePortalDirective;
  @ViewChild('DefaultHelp') defaultHelp: TemplatePortalDirective;
  @ViewChild('DefaultURLHelp') defaultURLHelp: TemplatePortalDirective;

  helpOverlayRef: OverlayRef;

  loadedHTML: string = '';

  backHistory: string[] = [];
  forwardHistory: string[] = [];
  currentContentUrl: string = '';

  constructor(protected controllerService: ControllerService,
    protected http: HttpClient,
    protected router: Router,
    protected overlay: Overlay) {}

  ngOnInit() {
    this.subscriptions.push( this.controllerService.subscribe(Eventtypes.HELP_CONTENT_CHANGE, (payload) => {
      this.setHelpContent();
    }));

    this.subscriptions.push( this.controllerService.subscribe(Eventtypes.HELP_OVERLAY_CLOSE, (payload) => {
      this.closeHelp();
    }));

    this.subscriptions.push( this.controllerService.subscribe(Eventtypes.HELP_OVERLAY_OPEN, (payload) => {
      this.openHelp();
    }));

    this.onUrlNavigation(this.router.url);
    this.router.events.pipe(filter(event => event instanceof NavigationEnd))
    .subscribe((event: NavigationEnd) => {
      this.onUrlNavigation(event.urlAfterRedirects);
    });
  }

  onUrlNavigation(url: string) {
    if (UrlMap[url]) {
      const id = UrlMap[url];
      this.replaceHelpContentByID(id);
      this.helpPortal = this.defaultURLHelp;
    } else {
      if (this.urlContentSubscription != null) {
        this.urlContentSubscription.unsubscribe();
      }
      this.currentContentUrl = '';
      this.helpPortal = this.defaultHelp;
    }
  }

  ngAfterViewInit() {
    this.helpOverlayRef = this.overlay.create({
      height: '400px',
      width: '400px',
      positionStrategy: new GlobalPositionStrategy().right('30px').top('100px'),
      panelClass: 'appcontent-help-panel'
    });
    this.setHelpContent();
  }

  setHelpContent() {
    // Commenting out for now since behavior for handeling
    // auto selecting help content by URL vs having components set
    // it isn't decided yet.

    // let data = this.controllerService.getHelpOverlayData();
    // if (data == null) {
    //   data = {};
    // }
    // if (data.component != null) {
    //   this.helpPortal = data.component;
    // } else if (data.template != null) {
    //   this.helpPortal = data.template;
    // } else if (data.id) {
    //   this.replaceHelpContentByID(data.id);
    //   this.helpPortal = this.defaultURLHelp;
    // // } else if (this.currentContentUrl.length === 0) {
    //   // this.helpPortal = this.defaultHelp;
    // }
  }

  toggleHelp($event) {
    if (!this.helpOverlayRef.hasAttached()) {
      this.helpOverlayRef.attach(this.helpShell);
    }  else {
      this.helpOverlayRef.detach();
      this.cleanHistory();
    }
  }

  closeHelp() {
    if (this.helpOverlayRef.hasAttached()) {
      this.helpOverlayRef.detach();
      this.cleanHistory();
    }
  }

  openHelp() {
    if (!this.helpOverlayRef.hasAttached()) {
      this.helpOverlayRef.attach(this.helpShell);
      this.cleanHistory();
    }
  }

  cleanHistory() {
    this.backHistory = [];
    this.forwardHistory = [];
  }

  replaceHelpContentByID(id: string) {
    const link = HelpLinkMap[id.toLowerCase()];
    if (link == null) {
      console.error('Helpoverlay: Invalid Help ID');
    }
    this.replaceHelpContentByURL(link, HistoryEvent.new);
  }

  popArray(arr: any[]) {
    if (arr.length === 0) {
      return;
    }
    return arr.splice(arr.length - 1, 1)[0];
  }

  backClick() {
    this.replaceHelpContentByURL(this.backHistory[this.backHistory.length - 1], HistoryEvent.backward);
  }

  forwardClick() {
    this.replaceHelpContentByURL(this.forwardHistory[this.forwardHistory.length - 1], HistoryEvent.forward);
  }

  replaceHelpContentByURL(url: string, event: HistoryEvent) {
    if (url === this.currentContentUrl) {
      return;
    }
    if (this.urlContentSubscription != null) {
      this.urlContentSubscription.unsubscribe();
    }
    this.urlContentSubscription = this.http.get(url, {responseType: 'text'}).subscribe( (resp) => {
      switch (event) {
        case HistoryEvent.new:
          this.forwardHistory = [];
          if (this.currentContentUrl.length !== 0) {
            this.backHistory.push(url);
          }
          break;
        case HistoryEvent.backward:
          this.popArray(this.backHistory);
          if (this.currentContentUrl.length !== 0) {
            this.forwardHistory.push(url);
          }
          break;
        case HistoryEvent.forward:
          this.popArray(this.forwardHistory);
          if (this.currentContentUrl.length !== 0) {
            this.backHistory.push(url);
          }
          break;
        default:
          break;
      }

      this.currentContentUrl = url;
      this.loadedHTML = resp;
      const contentElem = document.getElementsByClassName('helpoverlay-content');
      if (contentElem != null && contentElem.length !== 0) {
        contentElem[0].scrollTop = 0;
      }
      this.helpPortal = this.defaultURLHelp;
    });
    this.subscriptions.push(this.urlContentSubscription);
  }

  // We intercept all clicks in the help overlay component.
  // If they are on an href under assets, we block it, fetch
  // the content, and load it in the help window.
  hrefClicked(event) {
    let targetElement;

    if ( ( event.srcElement.nodeName.toUpperCase() === 'A') ) {
        targetElement = event.srcElement;
    } else if ( event.srcElement.parentElement.nodeName.toUpperCase() === 'A' ) {
        targetElement = event.srcElement.parentElement;
    } else {
      return;
    }
    if (targetElement.href && targetElement.href.includes(window.location.hostname) && targetElement.href.includes('assets')) {
      event.stopPropagation();
      event.preventDefault();

      this.replaceHelpContentByURL(targetElement.href, HistoryEvent.new);
    }

  }

  isBackButtonDisabled() {
    return this.backHistory.length === 0;
  }

  isForwardButtonDisabled() {
    return this.forwardHistory.length === 0;
  }

  ngOnDestroy() {
    this.subscriptions.forEach((s) => {
      s.unsubscribe();
    });
  }
}

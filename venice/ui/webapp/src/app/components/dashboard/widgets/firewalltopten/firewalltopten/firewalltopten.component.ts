import { Component, OnDestroy, OnInit, ChangeDetectorRef, Input } from '@angular/core';
import { Animations } from '@app/animations';
import { HttpEventUtility } from '@app/common/HttpEventUtility';
import { Utility } from '@app/common/Utility';
import { RuleHitCount } from '@app/components/dashboard/widgets/firewalltopten/TopRulesUtil';
import { ControllerService } from '@app/services/controller.service';
import { FwlogService } from '@app/services/generated/fwlog.service';
import { SecurityService } from '@app/services/generated/security.service';
import { MetricsqueryService, TelemetryPollingMetricQueries } from '@app/services/metricsquery.service';
import { ISecurityNetworkSecurityPolicyList, SecurityNetworkSecurityPolicy, ISecuritySGRule } from '@sdk/v1/models/generated/security';
import { ITelemetry_queryMetricsQueryResponse, ITelemetry_queryMetricsQuerySpec, Telemetry_queryMetricsQuerySpec_function, Telemetry_queryMetricsQuerySpec_sort_order } from '@sdk/v1/models/generated/telemetry_query';
import { UIChart } from 'primeng/primeng';
import { Subscription } from 'rxjs';

const MS_PER_MINUTE: number = 60000;
const QUERY_MX_RESULTS: number = 8192;

@Component({
  selector: 'app-firewalltopten',
  templateUrl: './firewalltopten.component.html',
  styleUrls: ['./firewalltopten.component.scss'],
  animations: [Animations]
})
export class FirewalltoptenComponent implements OnInit, OnDestroy {

  // Holds all policy objects
  sgPoliciesEventUtility: HttpEventUtility<SecurityNetworkSecurityPolicy>;
  sgPolicies: ReadonlyArray<SecurityNetworkSecurityPolicy> = [];
  ruleMap: any = {};
  ruleCountLists: { [key: string]: RuleHitCount[] } = {'allow': [], 'deny': [], 'reject': []};

  themeColor: string = '#61b3a0';
  subscriptions: Subscription[] = [];
  loading: boolean = false;


  @Input() updateInterval: number = 5 * 60000;

  constructor(
    protected fwlogService: FwlogService,
    protected controllerService: ControllerService,
    protected metricsqueryService: MetricsqueryService,
    protected securityService: SecurityService,
    protected cd: ChangeDetectorRef
    ) { }

  ngOnInit() {
    this.loading = true;
    this.getSecurityPolicies();
  }

  getRuleMetrics() {
    const queryList: TelemetryPollingMetricQueries = {
      queries: [],
      tenant: Utility.getInstance().getTenant()
    };
    const query: ITelemetry_queryMetricsQuerySpec = {
      'kind': 'RuleMetrics',
      function: Telemetry_queryMetricsQuerySpec_function.none,
      'sort-order': Telemetry_queryMetricsQuerySpec_sort_order.descending,
      'group-by-field': 'name',
      'start-time': 'now() - 5m' as any,
      'end-time': 'now()' as any,
      fields: [],
    };
    queryList.queries.push({ query: query });
    const sub = this.metricsqueryService.pollMetrics('firewallTopTen', queryList, this.updateInterval).subscribe(
      (data: ITelemetry_queryMetricsQueryResponse) => {
        if (data && data.results && data.results.length) {
          this.loading = true;
          this.cd.detectChanges();
          const rules = data.results[0].series;
          const totalHitsIndex = rules[0].columns.indexOf('TotalHits');
          this.ruleCountLists = {'allow': [], 'deny': [], 'reject': []};

          rules.forEach((rule) => {
            let totalHitCount = 0;
            const ruleHash = rule.tags['name'];
            rule.values.forEach((v) => {
            totalHitCount += v[totalHitsIndex];
            });
            if (totalHitCount > 0 && this.ruleMap[ruleHash]) {
              this.ruleCountLists[this.ruleMap[ruleHash].action].push(new RuleHitCount(ruleHash, totalHitCount));
            }
          });
          this.loading = false;
          this.cd.detectChanges();
        }
      },
    );
    this.subscriptions.push(sub);
  }

  getSecurityPolicies() {
    this.securityService.ListNetworkSecurityPolicy().subscribe(
      (response) => {
        if (response && response.body) {
          const body: ISecurityNetworkSecurityPolicyList = response.body as ISecurityNetworkSecurityPolicyList;
          if (body.items && body.items.length) {
            this.buildPolicyMaps(body.items);
            this.getRuleMetrics();
          }
        }
      },
      (error) => {
        this.controllerService.invokeRESTErrorToaster('Failed to get network security policy', error);
      }
    );
    this.sgPoliciesEventUtility = new HttpEventUtility<SecurityNetworkSecurityPolicy>(SecurityNetworkSecurityPolicy);
    this.sgPolicies = this.sgPoliciesEventUtility.array;
    const subscription = this.securityService.WatchNetworkSecurityPolicy().subscribe(
      response => {
        this.sgPoliciesEventUtility.processEvents(response);
        this.buildPolicyMaps(this.sgPolicies);
      },
      this.controllerService.webSocketErrorHandler('Failed to get security policies')
    );
    this.subscriptions.push(subscription);
  }

  buildPolicyMaps(policies) {
    // Since we currently only support single policy, we only use rulehash for identification
    // In future they should use policy name(or id) along with rulehash
    this.ruleMap = {};
    const rules = policies[0].spec.rules;
    const ruleHashes = policies[0].status['rule-status'];

    for (let ix = 0; ix < rules.length; ix++) {
      const rule = rules[ix];
      if (rule.action === 'permit') {
        this.ruleMap[ruleHashes[ix]['rule-hash']] = {'ruleIndex': ix + 1, 'action': 'allow', rule: rule as ISecuritySGRule };
      } else {
        this.ruleMap[ruleHashes[ix]['rule-hash']] = {'ruleIndex': ix + 1, 'action': rule.action, rule: rule as ISecuritySGRule };
      }

    }
  }

  ngOnDestroy() {
    this.subscriptions.forEach(subscription => subscription.unsubscribe());
  }

}

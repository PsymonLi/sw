/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */

// generate enum based on strings instead of numbers
// (see https://blog.rsuter.com/how-to-implement-an-enum-with-string-values-in-typescript/)
export enum ApiListWatchOptions_sort_order {
    'none' = "none",
    'by-name' = "by-name",
    'by-name-reverse' = "by-name-reverse",
    'by-version' = "by-version",
    'by-version-reverse' = "by-version-reverse",
    'by-creation-time' = "by-creation-time",
    'by-creation-time-reverse' = "by-creation-time-reverse",
    'by-mod-time' = "by-mod-time",
    'by-mod-time-reverse' = "by-mod-time-reverse",
}

export enum LabelsRequirement_operator {
    'equals' = "equals",
    'notequals' = "notequals",
    'in' = "in",
    'notin' = "notin",
}

export enum RolloutRolloutActionStatus_state {
    'progressing' = "progressing",
    'failure' = "failure",
    'success' = "success",
    'scheduled' = "scheduled",
    'suspended' = "suspended",
    'suspend-in-progress' = "suspend-in-progress",
    'deadline-exceeded' = "deadline-exceeded",
    'precheck-in-progress' = "precheck-in-progress",
    'scheduled-for-retry' = "scheduled-for-retry",
}

export enum RolloutRolloutPhase_phase {
    'pre-check' = "pre-check",
    'dependencies-check' = "dependencies-check",
    'waiting-for-turn' = "waiting-for-turn",
    'progressing' = "progressing",
    'complete' = "complete",
    'fail' = "fail",
}

export enum RolloutRolloutSpec_strategy {
    'linear' = "linear",
    'exponential' = "exponential",
}

export enum RolloutRolloutSpec_upgrade_type {
    'disruptive' = "disruptive",
    'on-next-reboot' = "on-next-reboot",
}

export enum RolloutRolloutStatus_state {
    'progressing' = "progressing",
    'failure' = "failure",
    'success' = "success",
    'scheduled' = "scheduled",
    'suspended' = "suspended",
    'suspend-in-progress' = "suspend-in-progress",
    'deadline-exceeded' = "deadline-exceeded",
    'precheck-in-progress' = "precheck-in-progress",
    'scheduled-for-retry' = "scheduled-for-retry",
}


export enum ApiListWatchOptions_sort_order_uihint {
    'by-creation-time' = "By Creation Time",
    'by-creation-time-reverse' = "By Creation Time Reverse",
    'by-mod-time' = "By Modification Time",
    'by-mod-time-reverse' = "By Modification Time Reverse",
    'by-name' = "By Name",
    'by-name-reverse' = "By Name Reverse",
    'by-version' = "By Version",
    'by-version-reverse' = "By Version Reverse",
    'none' = "None",
}

export enum LabelsRequirement_operator_uihint {
    'notequals' = "not equals",
    'notin' = "not in",
}

export enum RolloutRolloutPhase_phase_uihint {
    'complete' = "Complete",
    'dependencies-check' = "DependenciesCheck",
    'fail' = "Fail",
    'pre-check' = "PreCheck",
    'progressing' = "Progressing",
    'waiting-for-turn' = "WaitingForTurn",
}

export enum RolloutRolloutSpec_strategy_uihint {
    'exponential' = "Exponential",
    'linear' = "Linear",
}

export enum RolloutRolloutSpec_upgrade_type_uihint {
    'disruptive' = "Disruptive",
    'on-next-reboot' = "OnNextHostReboot",
}




/**
 * bundle of all enums for databinding to options, radio-buttons etc.
 * usage in component:
 *   import { AllEnums, minValueValidator, maxValueValidator } from '../../models/webapi';
 *
 *   @Component({
 *       ...
 *   })
 *   export class xxxComponent implements OnInit {
 *       allEnums = AllEnums;
 *       ...
 *       ngOnInit() {
 *           this.allEnums = AllEnums.instance;
 *       }
 *   }
*/
export class AllEnums {
    private static _instance: AllEnums = new AllEnums();
    constructor() {
        if (AllEnums._instance) {
            throw new Error("Error: Instantiation failed: Use AllEnums.instance instead of new");
        }
        AllEnums._instance = this;
    }
    static get instance(): AllEnums {
        return AllEnums._instance;
    }

    ApiListWatchOptions_sort_order = ApiListWatchOptions_sort_order;
    LabelsRequirement_operator = LabelsRequirement_operator;
    RolloutRolloutActionStatus_state = RolloutRolloutActionStatus_state;
    RolloutRolloutPhase_phase = RolloutRolloutPhase_phase;
    RolloutRolloutSpec_strategy = RolloutRolloutSpec_strategy;
    RolloutRolloutSpec_upgrade_type = RolloutRolloutSpec_upgrade_type;
    RolloutRolloutStatus_state = RolloutRolloutStatus_state;

    ApiListWatchOptions_sort_order_uihint = ApiListWatchOptions_sort_order_uihint;
    LabelsRequirement_operator_uihint = LabelsRequirement_operator_uihint;
    RolloutRolloutPhase_phase_uihint = RolloutRolloutPhase_phase_uihint;
    RolloutRolloutSpec_strategy_uihint = RolloutRolloutSpec_strategy_uihint;
    RolloutRolloutSpec_upgrade_type_uihint = RolloutRolloutSpec_upgrade_type_uihint;
}

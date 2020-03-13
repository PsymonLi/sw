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

export enum WorkloadEndpointMigrationStatus_status {
    'none' = "none",
    'start' = "start",
    'done' = "done",
    'failed' = "failed",
    'aborted' = "aborted",
    'from_non_pen_host' = "from_non_pen_host",
}

export enum WorkloadWorkloadMigrationStatus_stage {
    'migration-none' = "migration-none",
    'migration-start' = "migration-start",
    'migration-final-sync' = "migration-final-sync",
    'migration-done' = "migration-done",
    'migration-abort' = "migration-abort",
    'migration-from-non-pen-host' = "migration-from-non-pen-host",
}

export enum WorkloadWorkloadMigrationStatus_status {
    'none' = "none",
    'started' = "started",
    'done' = "done",
    'failed' = "failed",
    'timed-out' = "timed-out",
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

export enum WorkloadEndpointMigrationStatus_status_uihint {
    'done' = "Done",
    'failed' = "Failed",
    'none' = "None",
    'start' = "Start",
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
    WorkloadEndpointMigrationStatus_status = WorkloadEndpointMigrationStatus_status;
    WorkloadWorkloadMigrationStatus_stage = WorkloadWorkloadMigrationStatus_stage;
    WorkloadWorkloadMigrationStatus_status = WorkloadWorkloadMigrationStatus_status;

    ApiListWatchOptions_sort_order_uihint = ApiListWatchOptions_sort_order_uihint;
    WorkloadEndpointMigrationStatus_status_uihint = WorkloadEndpointMigrationStatus_status_uihint;
}

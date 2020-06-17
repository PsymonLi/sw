"use strict";
var __extends = (this && this.__extends) || (function () {
    var extendStatics = function (d, b) {
        extendStatics = Object.setPrototypeOf ||
            ({ __proto__: [] } instanceof Array && function (d, b) { d.__proto__ = b; }) ||
            function (d, b) { for (var p in b) if (b.hasOwnProperty(p)) d[p] = b[p]; };
        return extendStatics(d, b);
    };
    return function (d, b) {
        extendStatics(d, b);
        function __() { this.constructor = d; }
        d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
    };
})();
Object.defineProperty(exports, "__esModule", { value: true });
/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
var forms_1 = require("@angular/forms");
var validators_1 = require("../../../utils/validators");
var base_model_1 = require("../basemodel/base-model");
var telemetry_query_metrics_query_result_model_1 = require("./telemetry-query-metrics-query-result.model");
var Telemetry_queryMetricsQueryResponse = /** @class */ (function (_super) {
    __extends(Telemetry_queryMetricsQueryResponse, _super);
    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    function Telemetry_queryMetricsQueryResponse(values, setDefaults) {
        if (setDefaults === void 0) { setDefaults = true; }
        var _this = _super.call(this) || this;
        /** Field for holding arbitrary ui state */
        _this['_ui'] = {};
        /** Tenant for the request. */
        _this['tenant'] = null;
        /** Namespace for the request. */
        _this['namespace'] = null;
        _this['results'] = null;
        _this['results'] = new Array();
        _this._inputValue = values;
        _this.setValues(values, setDefaults);
        return _this;
    }
    Telemetry_queryMetricsQueryResponse.prototype.getPropInfo = function (propName) {
        return Telemetry_queryMetricsQueryResponse.propInfo[propName];
    };
    Telemetry_queryMetricsQueryResponse.prototype.getPropInfoConfig = function () {
        return Telemetry_queryMetricsQueryResponse.propInfo;
    };
    /**
     * Returns whether or not there is an enum property with a default value
    */
    Telemetry_queryMetricsQueryResponse.hasDefaultValue = function (prop) {
        return (Telemetry_queryMetricsQueryResponse.propInfo[prop] != null &&
            Telemetry_queryMetricsQueryResponse.propInfo[prop].default != null);
    };
    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    Telemetry_queryMetricsQueryResponse.prototype.setValues = function (values, fillDefaults) {
        if (fillDefaults === void 0) { fillDefaults = true; }
        if (values && values['_ui']) {
            this['_ui'] = values['_ui'];
        }
        if (values && values['tenant'] != null) {
            this['tenant'] = values['tenant'];
        }
        else if (fillDefaults && Telemetry_queryMetricsQueryResponse.hasDefaultValue('tenant')) {
            this['tenant'] = Telemetry_queryMetricsQueryResponse.propInfo['tenant'].default;
        }
        else {
            this['tenant'] = null;
        }
        if (values && values['namespace'] != null) {
            this['namespace'] = values['namespace'];
        }
        else if (fillDefaults && Telemetry_queryMetricsQueryResponse.hasDefaultValue('namespace')) {
            this['namespace'] = Telemetry_queryMetricsQueryResponse.propInfo['namespace'].default;
        }
        else {
            this['namespace'] = null;
        }
        if (values) {
            this.fillModelArray(this, 'results', values['results'], telemetry_query_metrics_query_result_model_1.Telemetry_queryMetricsQueryResult);
        }
        else {
            this['results'] = [];
        }
        this.setFormGroupValuesToBeModelValues();
    };
    Telemetry_queryMetricsQueryResponse.prototype.getFormGroup = function () {
        var _this = this;
        if (!this._formGroup) {
            this._formGroup = new forms_1.FormGroup({
                'tenant': validators_1.CustomFormControl(new forms_1.FormControl(this['tenant']), Telemetry_queryMetricsQueryResponse.propInfo['tenant']),
                'namespace': validators_1.CustomFormControl(new forms_1.FormControl(this['namespace']), Telemetry_queryMetricsQueryResponse.propInfo['namespace']),
                'results': new forms_1.FormArray([]),
            });
            // generate FormArray control elements
            this.fillFormArray('results', this['results'], telemetry_query_metrics_query_result_model_1.Telemetry_queryMetricsQueryResult);
            // We force recalculation of controls under a form group
            Object.keys(this._formGroup.get('results').controls).forEach(function (field) {
                var control = _this._formGroup.get('results').get(field);
                control.updateValueAndValidity();
            });
        }
        return this._formGroup;
    };
    Telemetry_queryMetricsQueryResponse.prototype.setModelToBeFormGroupValues = function () {
        this.setValues(this.$formGroup.value, false);
    };
    Telemetry_queryMetricsQueryResponse.prototype.setFormGroupValuesToBeModelValues = function () {
        if (this._formGroup) {
            this._formGroup.controls['tenant'].setValue(this['tenant']);
            this._formGroup.controls['namespace'].setValue(this['namespace']);
            this.fillModelArray(this, 'results', this['results'], telemetry_query_metrics_query_result_model_1.Telemetry_queryMetricsQueryResult);
        }
    };
    Telemetry_queryMetricsQueryResponse.propInfo = {
        'tenant': {
            description: "Tenant for the request.",
            required: false,
            type: 'string'
        },
        'namespace': {
            description: "Namespace for the request.",
            required: false,
            type: 'string'
        },
        'results': {
            required: false,
            type: 'object'
        },
    };
    return Telemetry_queryMetricsQueryResponse;
}(base_model_1.BaseModel));
exports.Telemetry_queryMetricsQueryResponse = Telemetry_queryMetricsQueryResponse;

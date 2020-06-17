"use strict";
/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
Object.defineProperty(exports, "__esModule", { value: true });
var forms_1 = require("@angular/forms");
var BaseModel = /** @class */ (function () {
    function BaseModel() {
        // define non enumerable properties so these are ommitted in JSON.stringify.
        Object.defineProperty(this, "$formGroup", {
            get: this.getFormGroup,
            enumerable: false,
        });
        Object.defineProperty(this, "_formGroup", {
            enumerable: false,
            writable: true,
        });
        Object.defineProperty(this, "$inputValue", {
            get: this.getInputValue,
            enumerable: false,
        });
        Object.defineProperty(this, "_inputValue", {
            writable: true,
            enumerable: false,
        });
        Object.defineProperty(this, "propInfo", {
            enumerable: false,
            writable: true,
        });
    }
    BaseModel.prototype.getInputValue = function () {
        return this._inputValue;
    };
    BaseModel.prototype.getFormGroupValues = function () {
        return this.$formGroup.value;
    };
    BaseModel.prototype.getModelValues = function () {
        return JSON.parse(JSON.stringify(this));
    };
    /**
     * add one or more additional validators to the control
     * @param key Name of the control (is the same as the name of the attached model property)
     * @param validators Validator(s) to add to the control
    */
    BaseModel.prototype.addValidatorToControl = function (key, validators) {
        var control = this.$formGroup.controls[key];
        var vals = validators instanceof Array ? validators : [validators];
        if (control.validator) {
            vals.push(control.validator);
        }
        control.setValidators(vals);
    };
    BaseModel.prototype.fillModelArray = function (object, key, values, type) {
        if (type === void 0) { type = undefined; }
        if (values) {
            object[key] = new Array();
            for (var _i = 0, values_1 = values; _i < values_1.length; _i++) {
                var value = values_1[_i];
                if (type) {
                    object[key].push(new type(value));
                }
                else {
                    object[key].push(value);
                }
            }
            // generate FormArray control elements
            this.fillFormArray(key, object[key], type);
        }
    };
    BaseModel.prototype.fillFormArray = function (key, modelValues, type) {
        if (type === void 0) { type = undefined; }
        if (this._formGroup) {
            var formArray = BaseModel.clearFormArray(this._formGroup, key);
            for (var _i = 0, modelValues_1 = modelValues; _i < modelValues_1.length; _i++) {
                var modelValue = modelValues_1[_i];
                if (type) {
                    formArray.push(modelValue.$formGroup);
                }
                else {
                    formArray.push(new forms_1.FormControl(modelValue));
                }
            }
        }
    };
    BaseModel.clearFormArray = function (formGroup, key) {
        if (formGroup) {
            var formArray = formGroup.controls[key];
            for (var i = formArray.length - 1; i >= 0; i--) {
                formArray.removeAt(i);
            }
            return formArray;
        }
    };
    return BaseModel;
}());
exports.BaseModel = BaseModel;

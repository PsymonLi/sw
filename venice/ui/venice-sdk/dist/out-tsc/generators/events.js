"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.generateEventTypesFile = void 0;
var utils = require("./utils");
function generateEventTypesFile(inputFile, outputFile) {
    var events = utils.readJSON(inputFile);
    var templateFile = 'generate-eventtypes-ts.hbs';
    utils.writeTemplate(templateFile, events, outputFile);
}
exports.generateEventTypesFile = generateEventTypesFile;

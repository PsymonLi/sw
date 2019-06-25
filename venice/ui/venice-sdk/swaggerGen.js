var fs = require( 'fs' );
var path = require( 'path' );
let handlebars = require('handlebars');
var rmdir = require('rimraf');
let _ = require('lodash');
let customMetrics = require('./custom-metrics');

var swaggerTSGenerator = require('@pensando/swagger-ts-generator/');

// Expects to run from root of venice-sdk

// Generate folders for generated files, and remove any files if there are any
const version = 'v1';
const genModelFolder = path.join(__dirname, 'v1/', 'models/generated/');
const genServiceFolder = path.join(__dirname, 'v1/', 'services/generated/');
const metricsFolder = path.join(__dirname, 'metrics/', 'generated/');

handlebars.registerHelper('toJSON', function (obj) {
  return JSON.stringify(obj, null, 2);
});
handlebars.registerHelper('ifeq', function (a, b, options) {
  if (a == b) { return options.fn(this); }
  return options.inverse(this);
});

handlebars.registerHelper('ifnoteq', function (a, b, options) {
  if (a != b) { return options.fn(this); }
  return options.inverse(this);
});

if (fs.existsSync(genModelFolder)){
  // Delete all contents
  rmdir.sync(genModelFolder);
}
fs.mkdirSync(genModelFolder);

if (fs.existsSync(genServiceFolder)){
  // Delete all contents
  rmdir.sync(genServiceFolder);
}
fs.mkdirSync(genServiceFolder);

if (fs.existsSync(metricsFolder)){
  // Delete all contents
  rmdir.sync(metricsFolder);
}
fs.mkdirSync(metricsFolder);

// Generating models and services
// Path to generated api files
// Expecting format to be /api/generated/*/swagger/svc_*.swagger.json
var generatedApi = path.join(__dirname, '../../../api/generated');
var generatedApiFiles = fs.readdirSync(generatedApi);
generatedApiFiles.forEach( (generatedApiFile) => {
  if (fs.statSync(generatedApi+'/'+generatedApiFile).isDirectory() && 
      fs.existsSync(generatedApi+'/'+generatedApiFile+'/swagger') &&
      fs.statSync(generatedApi+'/'+generatedApiFile+'/swagger').isDirectory()) {
    folderName = generatedApiFile;
    swaggerFiles = fs.readdirSync(generatedApi+'/'+generatedApiFile+'/swagger').filter(function (file) {
      return file.startsWith('svc_') && file.endsWith('.swagger.json');
    });
    swaggerFiles.forEach( (swaggerFile) => {
      let config;
      // Search models has special generation to add more helpful UI typing
      if (generatedApiFile === 'search') {
        config = genConfig(version, folderName, generatedApi+'/'+generatedApiFile+'/swagger/'+ swaggerFile, true);
      } else {
        config = genConfig(version, folderName, generatedApi+'/'+generatedApiFile+'/swagger/'+ swaggerFile);
      }
      if (generatedApiFile === 'events') {
        config.swagger.swaggerTSGeneratorOptions.swaggerFileHook = (swagger) => {
          eventTypes = [];
          const eventPath = path.join(__dirname, '../../../events/generated/eventtypes/eventtypes.json');
          const events = JSON.parse(fs.readFileSync(eventPath, 'utf8').trim());
          Object.keys(events).forEach((cat) => {
            events[cat].forEach( (entry) => {
              eventTypes.push(entry.Name);
            });
          });
          swagger.definitions.eventsEvent.properties.type.enum =  eventTypes;
        }
      }
      swaggerTSGenerator.generateTSFiles(config.swagger.swaggerFile, config.swagger.swaggerTSGeneratorOptions);
    })
  }
});
// Generate search cat-kind helper from svc_manifest.json
const manifestPath = path.join(__dirname, '../../../api/generated/apiclient/svcmanifest.json');
const manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8').trim());
const data = {};
Object.keys(manifest).forEach( (category) => {
  if (category === "bookstore") {
    // Skipping the bookstore exampe
    return;
  }
  // if (category === "staging") {
  //   // Skipping the staging category
  //   return;
  // }
  const internalServices = manifest[category]["Svcs"];
  // We assume only one internal service
  const serviceData = internalServices[Object.keys(internalServices)[0]];
  const version = serviceData.Version;
  if (data[version] == null) {
    data[version] = {};
  }
  if (data[version][_.upperFirst(category)] == null) {
    data[version][_.upperFirst(category)] = {};
  }
  const catObj = data[version][_.upperFirst(category)];
  serviceData.Messages.forEach( (kind) => {
    if (kind === "TroubleshootingSession") {
      return;
    }
    catObj[kind] = {
      // File will be placed in {Version}/models/generated/
      "importPath": "./" + category,
      "importName": _.upperFirst(category) + kind,
      scopes: serviceData.Properties[kind].Scopes,
      actions: serviceData.Properties[kind].Actions
    }
  });
  if (category === "monitoring") {
    catObj["Event"] = {
      "importPath": "./events",
      "importName": "EventsEvent",
    }
    catObj["AuditEvent"] = {
      "importPath": "./audit",
      "importName": "AuditEvent",
    }
  }
});


generateCatKind(data);
generateEventTypesFile(data);
generateUIPermissionsFile(manifest, data);

generateMetricMetadata();


// Recursively search under base for files with the given extension 
function recFindByExt(base,ext,files,result) 
{
    files = files || fs.readdirSync(base) 
    result = result || [] 

    files.forEach( 
        function (file) {
            var newbase = path.join(base,file)
            if ( fs.statSync(newbase).isDirectory() )
            {
                result = recFindByExt(newbase,ext,fs.readdirSync(newbase),result)
            }
            else
            {
                if ( file.substr(-1*(ext.length+1)) == '.' + ext )
                {
                    result.push(newbase)
                } 
            }
        }
    )
    return result
}

function readAndCompileTemplateFile(templateFileName) {
  let templateSource = fs.readFileSync(path.resolve(__dirname, "templates", templateFileName), 'utf8');
  let template = handlebars.compile(templateSource);
  return template;
}

function generateMetricMetadata() {
  const basetypeToJSType = {
    int8: 'number',
    uint8: 'number',
    int16: 'number',
    uint16: 'number',
    int32: 'number',
    uint32: 'number',
    int64: 'number',
    uint64: 'number',
    int: 'number',
    uint: 'number',
    Counter: 'number',
    float32: 'number',
    float64: 'number',
    complex64: 'number',
    complex128: 'number',
    byte: 'number',
    number: 'number',

    string: 'string',
  }

  files = recFindByExt('../../../metrics/generated/', 'json')
  messages = [];
  files.forEach( (f) => {
    const metadata = JSON.parse(fs.readFileSync(f, 'utf8').trim());
    // const prefix = metadata.prefix
    metadata.Messages.forEach((m) => {
      m = _.transform(m, function (result, val, key) {
        result[_.camelCase(key)] = val;
      });
      m.objectKind = "SmartNIC"
      if (m.fields == null) {
        return;
      }
      m.fields.push({
        Name: 'reporterID',
        Description: 'Name of reporting object',
        BaseType: 'string',
        JsType: 'string',
        IsTag: true,
      })
      m.fields = m.fields.map( (field) => {
        if (field.DisplayName == null) {
          field.DisplayName = field.Name;
        }
        if (field.Tags == null) {
          field.Tags = ['Level4']
        }
        if (basetypeToJSType[field.BaseType] == null) {
          throw new Error("base type " + field.BaseType + " not recognized for field " + field.Name);
        }
        field.jsType = basetypeToJSType[field.BaseType];
        return _.transform(field, function (result, val, key) {
          result[_.camelCase(key)] = val;
        });
      });
      messages.push(m);
    });
  });
  customMetrics.metrics.forEach( (metric) => {
    messages.push(metric);
  })
  const template = readAndCompileTemplateFile('generate-metrics-ts.hbs');
  const result = template(messages);
  const outputFileName = 'metrics/generated/metadata.ts';
  swaggerTSGenerator.utils.ensureFile(outputFileName, result);
  swaggerTSGenerator.utils.log(`generated ${outputFileName}`);
}

function generateCatKind(versionData) {
  Object.keys(versionData).forEach( (version) => {
    const template = readAndCompileTemplateFile('generate-cat-kind-ts.hbs');
    const result = template(versionData[version]);
    const outputFileName = version + '/models/generated/category-mapping.model.ts';
    swaggerTSGenerator.utils.ensureFile(outputFileName, result);
    swaggerTSGenerator.utils.log(`generated ${outputFileName}`);
  });

}

function generateEventTypesFile(versionData) {
  Object.keys(versionData).forEach( (version) => {
    const eventPath = path.join(__dirname, '../../../events/generated/eventtypes/eventtypes.json');
    const events = JSON.parse(fs.readFileSync(eventPath, 'utf8').trim());
    const template = readAndCompileTemplateFile('generate-eventtypes-ts.hbs');
    const result = template(events);
    const outputFileName = version + '/models/generated/eventtypes.ts';
    swaggerTSGenerator.utils.ensureFile(outputFileName, result);
    swaggerTSGenerator.utils.log(`generated ${outputFileName}`);
  });
}

function generateUIPermissionsFile(manifestData, versionData) {
  const delimiter = '_';
  const authPath = path.join(__dirname, '../../../api/generated/auth/swagger/svc_auth.swagger.json');
  const authSwagger = JSON.parse(fs.readFileSync(authPath, 'utf8').trim());
  const permActions = authSwagger.definitions.authPermission.properties.actions.enum
  const permEnum = [];
  Object.keys(manifestData).forEach( (category) => {
    if (category === "bookstore") {
      // Skipping the bookstore exampe
      return;
    }
    const internalServices = manifestData[category]["Svcs"];
    // We assume only one internal service
    const serviceData = internalServices[Object.keys(internalServices)[0]];

    serviceData.Messages.forEach( (kind) => {
      permActions.forEach( (action) => {
        permEnum.push( _.toLower(category) + _.toLower(kind) + delimiter + _.toLower(action));
      });
    });
  });
  const addCustomKind = (kind) => {
    permActions.forEach( (action) => {
      permEnum.push( _.toLower(kind) + delimiter + _.toLower(action));
    });
  }
  permEnum.push('auditevent' + delimiter + 'read');
  permEnum.push('eventsevent' + delimiter + 'read');
  permEnum.push('fwlogsquery' + delimiter + 'read');
  permEnum.push('adminrole');

  Object.keys(versionData).forEach( (version) => {
    const template = readAndCompileTemplateFile('generate-permissions-enum-ts.hbs');
    const result = template(permEnum);
    const outputFileName = version + '/models/generated/UI-permissions-enum.ts';
    swaggerTSGenerator.utils.ensureFile(outputFileName, result);
    swaggerTSGenerator.utils.log(`generated ${outputFileName}`);
  });
}

function genConfig(version, folderName, swaggerFile, isSearch = false) {
  var srcAppFolder = './' + version + '/';
  var folders = {
      srcWebapiFolder: srcAppFolder + 'models/generated/' + folderName,
      serviceFolder: srcAppFolder + 'services/generated/',
      serviceBaseImport: './abstract.service',
      controllerImport: '../../../../webapp/src/app/services/controller.service',
      modelImport: '../../models/generated/' + folderName,
  }
  var files = {
      swaggerJson: 'swagger.json',
  }

  var swagger = {
      swaggerFile: swaggerFile,
      swaggerTSGeneratorOptions: {
          isSearch: isSearch,
          // Service options
          modelImport: folders.modelImport,
          serviceFolder: folders.serviceFolder,
          serviceBaseImport: folders.serviceBaseImport,
          serviceBaseName:  'AbstractService',
          implicitParams: ['O_Tenant'],
          controllerImport: folders.controllerImport,
          // model options
          modelFolder: folders.srcWebapiFolder,
          enumTSFile: folders.srcWebapiFolder + '/enums.ts',
          generateClasses: true,
          modelModuleName: 'webapi.models',
          enumModuleName: 'webapi.enums',
          enumRef: './enums',
          namespacePrefixesToRemove: [
          ],
          typeNameSuffixesToRemove: [
          ],
          typesToFilter: [
              'ModelAndView', // Springfox artifact
              'View', // Springfox artifact
          ]
      }
  }

  var config = {
      files: files,
      swagger: swagger,
  }
  return config;
}


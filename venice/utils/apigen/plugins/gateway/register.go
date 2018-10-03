package plugin

import (
	"bytes"
	"encoding/json"
	"errors"
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"
	"regexp"
	"sort"
	"strconv"
	"strings"

	"github.com/golang/glog"

	gogoproto "github.com/gogo/protobuf/protoc-gen-gogo/descriptor"
	plugin "github.com/gogo/protobuf/protoc-gen-gogo/plugin"
	"github.com/pensando/grpc-gateway/protoc-gen-grpc-gateway/descriptor"
	reg "github.com/pensando/grpc-gateway/protoc-gen-grpc-gateway/plugins"
	googapi "github.com/pensando/grpc-gateway/third_party/googleapis/google/api"

	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/apigen/annotations"
	mutator "github.com/pensando/sw/venice/utils/apigen/autogrpc"
	"github.com/pensando/sw/venice/utils/apigen/plugins/common"
)

var (
	errIncompletePath = errors.New("incomplete path specification")
	errInvalidField   = errors.New("invalid field specification")
	errInvalidOption  = errors.New("invalid option specification")
)

type scratchVars struct {
	B   [3]bool
	Int [3]int
	Str [3]string
}

var scratch scratchVars

func (s *scratchVars) setBool(val bool, id int) bool {
	s.B[id] = val
	// Dummy return to satisfy templates
	return val
}

func (s *scratchVars) getBool(id int) bool {
	return s.B[id]
}

func (s *scratchVars) setInt(val int, id int) int {
	s.Int[id] = val
	return val
}

func (s *scratchVars) getInt(id int) int {
	return s.Int[id]
}

func (s *scratchVars) setStr(val string, id int) string {
	s.Str[id] = val
	return val
}

func (s *scratchVars) getStr(id int) string {
	return s.Str[id]
}

// ServiceParams is the parameters related to the Service used by templates
type ServiceParams struct {
	// Version is the version of the Service
	Version string
	// Prefix is the prefix for all the resources served by the service.
	Prefix string
	// URIPath is the URI Path prefix for this service. This is combination of
	// Version and Prefix and the catogory that is inherited from
	//  the fileCategory options specified at the file level
	URIPath string
	// StagingPath is the URI path prefix for this service if it supports config
	//  staging. If the service does not support staging then it is empty.
	StagingPath string
}

// RestServiceOptions holds raw REST options data from .proto files
type RestServiceOptions struct {
	CrudObject string
	Methods    []string
	Pattern    string
}

// MethodParams is the parameters related to a method used by templates
type MethodParams struct {
	// GrpcOnly marks if this method has no REST bindings.
	GrpcOnly bool
	// Oper specifies the type of CRUD operation, valid with GrpcOnly flag.
	Oper string
	// TenantDefault marks that this method has a flavor with inferred tenant default.
	TenantDefault bool
}

// KeyComponent is a component of the key path, derived from the objectPrefix option.
type KeyComponent struct {
	// Type of the compoent - prefix or field
	Type string
	// Val holds a string literal or a field name depending on type
	Val string
}

//--- Functions registered in the funcMap for the plugin  ---//

// findComponentsHelper is a helper function to derive a KeyComponent slice from a string.
func findComponentsHelper(m *descriptor.Message, input string) ([]KeyComponent, error) {
	var output []KeyComponent
	in := input
	for len(in) > 0 {
		s := strings.SplitN(in, "{", 2)
		if s[0] != "" {
			output = append(output, KeyComponent{Type: "prefix", Val: s[0]})
		}
		if len(s) < 2 {
			break
		}

		s = strings.SplitN(s[1], "}", 2)
		if len(s) < 2 {
			glog.V(1).Infof("Found incomplete path processing [%s]", input)
			return output, errIncompletePath
		}
		var finalMd *descriptor.Message
		var field string
		var ok, embedded bool
		if i := strings.LastIndex(s[0], "."); i != -1 {
			pre := s[0][:i]
			field = s[0][i+1:]
			glog.V(1).Infof("Found Pre/Field to be %v/%v", pre, field)

			finalMd, embedded, ok = m.FindMessageField(pre)
			if !ok {
				glog.V(1).Infof("Did not find %v in [%v]", pre, m.GetName())
				return nil, errInvalidField
			}
		} else {
			finalMd = m
			field = s[0]
		}

		if _, _, ok := finalMd.FindMessageField(field); ok {
			glog.V(1).Infof("Found #1 Pre/Field to be %v/%v", field, embedded)
			if embedded {
				output = append(output, KeyComponent{Type: "field", Val: field})
			} else {
				output = append(output, KeyComponent{Type: "field", Val: s[0]})
			}
		} else {
			glog.Errorf("Invalid Field [%s]", field)
			return output, errInvalidField
		}
		if s[0] == "" {
			glog.V(1).Infof("Found incomplete path processing [%s]", input)
			return output, errIncompletePath
		}
		in = s[1]
	}
	return output, nil
}

// getDbKey returns a slice of KeyComponents for the message that is used by
// templates to construct the key.
func getDbKey(m *descriptor.Message) ([]KeyComponent, error) {
	var output []KeyComponent
	var err error
	dbpath, err := reg.GetExtension("venice.objectPrefix", m)
	if err != nil {
		return output, nil
	}
	in := dbpath.(*venice.ObjectPrefix)
	prefx := ""
	singleton := false
	if prefx = in.GetCollection(); prefx != "" {
		prefx = strings.TrimPrefix(prefx, "/")
	} else if prefx = in.GetSingleton(); prefx != "" {
		singleton = true
		prefx = strings.TrimPrefix(prefx, "/")
	}
	if len(prefx) == 0 {
		return output, nil
	}
	if in.Path != "" {
		prefx = prefx + "/"
		path := strings.TrimSuffix(in.Path, "/")
		path = strings.TrimPrefix(path, "/")
		prefx = prefx + path

	}
	if !singleton {
		prefx = prefx + "/"
	}
	if output, err = findComponentsHelper(m, prefx); err == nil {
		// The key generated is /venice/<service prefix>/<object prefix>/<name from object meta>
		if !singleton {
			output = append(output, KeyComponent{Type: "field", Val: "Name"})
		} else {
			output = append(output, KeyComponent{Type: "prefix", Val: "/Singleton"})
		}
	}

	glog.V(1).Infof("Built DB key [ %v ](%v)", output, err)
	return output, err
}

// URIKey specifies a URI
type URIKey struct {
	// Str is the string for for the URI
	Str string
	// Ref is set to true if any references to the object exist in the URI
	Ref bool
}

func getMsgURIKey(m *descriptor.Message, prefix string) (URIKey, error) {
	var out URIKey
	var output []KeyComponent
	var err error

	if output, err = getMsgURI(m, "", prefix); err != nil {
		return out, nil
	}
	out.Str = ""
	out.Ref = false
	sep := ""
	for _, v := range output {
		if v.Type == "prefix" {
			out.Str = fmt.Sprintf("%s%s\"%s\"", out.Str, sep, v.Val)
		} else if v.Type == "field" {
			out.Ref = true
			out.Str = fmt.Sprintf("%s%sin.%s", out.Str, sep, v.Val)
		}
		sep = ", "
	}
	return out, nil
}

// getURIKey gets the URI key given the method. The req parameter specifies
//  if this is in the req direction or resp. In the response direction the URI
//  is always the URI that can be used to access the object.
func getURIKey(m *descriptor.Method, ver string, req bool) (URIKey, error) {
	var output []KeyComponent
	var out URIKey

	params, err := getMethodParams(m)
	if err != nil {
		return out, err
	}
	msg := m.RequestType
	if params.Oper == "ListOper" || params.Oper == "WatchOper" {
		msgtype := ""
		if params.Oper == "ListOper" {
			msgtype, err = getListType(m.ResponseType, true)
			if err != nil {
				return out, err
			}
			msgtype = "." + msgtype
		}
		if params.Oper == "WatchOper" {
			msgtype, err = getWatchType(m.ResponseType, true)
			if err != nil {
				return out, err
			}
			msgtype = "." + msgtype
		}
		msg, err = m.Service.File.Reg.LookupMsg("", msgtype)
		if err != nil {
			return out, err
		}
	}
	svcParams, err := getSvcParams(m.Service)
	if err != nil {
		return out, err
	}
	if req {
		r, err := reg.GetExtension("google.api.http", m)
		rule := r.(*googapi.HttpRule)
		pattern := ""
		switch params.Oper {
		case "CreateOper":
			pattern = rule.GetPost()
		case "GetOper", "ListOper":
			pattern = rule.GetGet()
		case "DeleteOper":
			pattern = rule.GetDelete()
		case "UpdateOper":
			pattern = rule.GetPut()
		}
		if output, err = findComponentsHelper(msg, pattern); err != nil {
			return out, err
		}
	} else {
		if output, err = getMsgURI(msg, ver, svcParams.Prefix); err != nil {
			return out, err
		}
	}

	out.Str = ""
	out.Ref = false
	sep := ""
	for _, v := range output {
		if v.Type == "prefix" {
			out.Str = fmt.Sprintf("%s%s\"%s\"", out.Str, sep, v.Val)
		} else if v.Type == "field" {
			out.Ref = true
			out.Str = fmt.Sprintf("%s%sin.%s", out.Str, sep, v.Val)
		}
		sep = ", "
	}
	return out, nil
}

// getMsgURI returns the key for the Message URI
func getMsgURI(m *descriptor.Message, ver, svcPrefix string) ([]KeyComponent, error) {
	var output []KeyComponent
	str, err := mutator.GetMessageURI(m.DescriptorProto)
	if err != nil {
		return output, err
	}
	svcPrefix = strings.TrimSuffix(svcPrefix, "/")
	svcPrefix = strings.TrimPrefix(svcPrefix, "/")
	if ver != "" {
		str = svcPrefix + "/" + ver + str
	} else {
		str = svcPrefix + str
	}
	if output, err = findComponentsHelper(m, str); err != nil {
		return output, err
	}
	return output, nil
}

// getSvcParams returns the ServiceParams for the service.
//   Parameters could be initialized to defaults if options were
//   not specified by the user in service.proto.
func getSvcParams(s *descriptor.Service) (ServiceParams, error) {
	var params ServiceParams
	var ok bool
	i, err := reg.GetExtension("venice.apiVersion", s)
	if params.Version, ok = i.(string); err != nil || !ok {
		// Can specify a defaults when not specified.
		params.Version = ""
	}
	i, err = reg.GetExtension("venice.apiPrefix", s)
	if params.Prefix, ok = i.(string); err != nil || !ok {
		params.Prefix = ""
	}
	glog.V(1).Infof("Looking for File category")
	category := globals.ConfigURIPrefix
	if i, err = reg.GetExtension("venice.fileCategory", s.File); err == nil {
		if category, ok = i.(string); !ok {
			category = globals.ConfigURIPrefix
		}
	} else {
		glog.V(1).Infof("Did not find Category %s", err)
	}
	if params.Prefix == "" {
		params.URIPath = "/" + category + "/" + params.Version
		if category == globals.ConfigURIPrefix {
			params.StagingPath = "/staging/{{TOCTX.BufferId}}/" + params.Version
		}
	} else {
		params.URIPath = "/" + category + "/" + params.Prefix + "/" + params.Version
		if category == globals.ConfigURIPrefix {
			params.StagingPath = "/staging/{TOCTX.BufferId}/" + params.Prefix + "/" + params.Version
		}
	}
	return params, nil
}

// getRestSvcOptions returns the ServiceOptions for the service. This call will ensure that the raw venice.naplesRestService
// is passed to the templating logic for customization. This will also avoid generating the *.pb.gw files if we don't
// want them.
func getRestSvcOptions(s *descriptor.Service) ([]RestServiceOptions, error) {
	var restOptions []RestServiceOptions
	//var ok bool
	i, _ := reg.GetExtension("venice.naplesRestService", s)
	for _, r := range i.([]*venice.RestEndpoint) {
		var restService RestServiceOptions
		restService.CrudObject = r.Object
		restService.Methods = r.Method
		restService.Pattern = r.Pattern
		restOptions = append(restOptions, restService)
	}
	glog.V(1).Infof("RestAPIParsing yielded : %#v", restOptions)
	return restOptions, nil
}

// getMethodParams returns params for the method in a MethodParams object.
//   Parameters could be initialized to defaults if options were
//   not specified by the user in service.proto.
func getMethodParams(m *descriptor.Method) (MethodParams, error) {
	var params MethodParams
	params.GrpcOnly = true
	ok := false
	i, err := reg.GetExtension("venice.methodOper", m)
	if err != nil {
		glog.V(1).Infof("GrpcOnly but no Method specified(%s)", *m.Name)
		return params, nil
	}
	if d, err := reg.GetExtension("venice.methodTenantDefault", m); err == nil {
		params.TenantDefault = d.(bool)
	}
	if params.Oper, ok = i.(string); !ok {
		return params, errInvalidOption
	}
	switch strings.ToLower(params.Oper) {
	case "create":
		params.Oper = "CreateOper"
	case "update":
		params.Oper = "UpdateOper"
	case "get":
		params.Oper = "GetOper"
	case "delete":
		params.Oper = "DeleteOper"
	case "list":
		params.Oper = "ListOper"
	case "watch":
		params.Oper = "WatchOper"
	default:
		return params, errInvalidOption
	}
	return params, nil
}

func getSwaggerFileName(file string) (string, error) {
	file = filepath.Base(file)
	if strings.HasSuffix(file, ".proto") {
		f := strings.TrimSuffix(file, ".proto")
		return f + ".swagger.json", nil
	}
	return "", errInvalidOption
}

// getCWD2 returns the cwd working directory but qualified by the parent directory.
func getCWD2() string {
	cwd, err := os.Getwd()
	if err != nil {
		return ""
	}
	return filepath.Base(filepath.Dir(cwd)) + "/" + filepath.Base(cwd)
}

// createDir creates a directory at the base given.
func createDir(base string, dirs ...string) error {
	name := base
	for _, d := range dirs {
		name = name + "/" + d
	}
	os.MkdirAll(name, 0744)
	return nil
}

type manifestFile struct {
	Pkg       string
	APIServer bool
}

func parseManifestFile(raw []byte) map[string]manifestFile {
	manifest := make(map[string]manifestFile)
	lines := bytes.Split(raw, []byte("\n"))
	for _, line := range lines {
		fields := bytes.Fields(line)
		if len(fields) == 3 {
			apiserver, err := strconv.ParseBool(string(fields[2]))
			if err == nil {
				manifest[string(fields[0])] = manifestFile{
					Pkg:       string(fields[1]),
					APIServer: apiserver,
				}
			}
		}
	}
	return manifest
}

// genManifest generates the current manifest of protos being processed.
func genManifest(desc *descriptor.File, path, pkg, file string) (map[string]manifestFile, error) {
	var manifest map[string]manifestFile
	if _, err := os.Stat(path); os.IsNotExist(err) {
		glog.V(1).Infof("manifest [%s] not found", path)
		manifest = make(map[string]manifestFile)
	} else {
		glog.V(1).Infof("manifest exists, reading from manifest")
		raw, err := ioutil.ReadFile(path)
		if err != nil {
			glog.V(1).Infof("Reading Manifest failed (%s)", err)
			return nil, err
		}
		manifest = parseManifestFile(raw)
	}
	apiserver, _ := isAPIServerServed(desc)
	file = filepath.Base(file)
	manifest[file] = manifestFile{
		Pkg:       pkg,
		APIServer: apiserver,
	}
	return manifest, nil
}

type pkgManifest struct {
	Files     []string
	APIServer bool
}

func parsePkgManifest(raw []byte) map[string]pkgManifest {
	manifest := make(map[string]pkgManifest)
	lines := bytes.Split(raw, []byte("\n"))
	for _, line := range lines {
		fields := bytes.Fields(line)
		if len(fields) > 2 {
			apiserver, err := strconv.ParseBool(string(fields[0]))
			if err != nil {
				glog.Fatalf("malformed pkg manifest [%s]", string(line))
			}
			pkg := string(fields[1])
			files := []string{}
			for i := 2; i < len(fields); i++ {
				files = append(files, string(fields[i]))
			}
			manifest[pkg] = pkgManifest{
				APIServer: apiserver,
				Files:     files,
			}
		}
	}
	return manifest
}

func genPkgManifest(desc *descriptor.File, path, pkg, file string) (map[string]pkgManifest, error) {
	var manifest map[string]pkgManifest
	if _, err := os.Stat(path); os.IsNotExist(err) {
		glog.V(1).Infof("manifest [%s] not found", path)
		manifest = make(map[string]pkgManifest)
	} else {
		glog.V(1).Infof("manifest exists, reading from manifest")
		raw, err := ioutil.ReadFile(path)
		if err != nil {
			glog.V(1).Infof("Reading Manifest failed (%s)", err)
			return nil, err
		}
		manifest = parsePkgManifest(raw)
	}
	apiserver, _ := isAPIServerServed(desc)
	file = filepath.Base(file)
	var ok bool
	var m pkgManifest
	if m, ok = manifest[pkg]; !ok {
		m = pkgManifest{APIServer: apiserver}
	}
	found := false
	for i := range m.Files {
		if m.Files[i] == file {
			found = true
			break
		}
	}
	if !found {
		m.Files = append(m.Files, file)
	}

	manifest[pkg] = m
	return manifest, nil
}

//
type packageDef struct {
	Svcs  map[string]serviceDef
	Files []string
}

type serviceDef struct {
	Version  string
	Messages []string
}

// getServiceManifest retrieves the manifest from file specified in arg
func getServiceManifest(filenm string) (map[string]packageDef, error) {
	manifest := make(map[string]packageDef)
	if _, err := os.Stat(filenm); os.IsNotExist(err) {
		glog.V(1).Infof("manifest [%s] not found", filenm)
		return manifest, nil
	}
	glog.V(1).Infof("manifest exists, reading from manifest")
	raw, err := ioutil.ReadFile(filenm)
	if err != nil {
		glog.V(1).Infof("Reading Manifest failed (%s)", err)
		return nil, err
	}
	err = json.Unmarshal(raw, &manifest)
	if err != nil {
		glog.V(1).Infof("Json Unmarshall of svc manifest file failed ignoring current file")
		return nil, err
	}
	glog.V(1).Infof("manifest has %v packages", len(manifest))
	return manifest, nil
}

// genServiceManifest generates a service manifest at the location specified.
//  The manifest is created in an additive fashion since it is called once per protofile.
func genServiceManifest(filenm string, file *descriptor.File) (string, error) {
	manifest, err := getServiceManifest(filenm)
	if err != nil {
		return "", err
	}
	pkg := file.GoPkg.Name
	var pkgdef packageDef
	ok := false
	if pkgdef, ok = manifest[pkg]; !ok {
		pkgdef = packageDef{Svcs: make(map[string]serviceDef)}
	}
	pkgdef.Files = append(pkgdef.Files, *file.Name)
	for _, svc := range file.Services {
		ver, err := reg.GetExtension("venice.apiVersion", svc)
		if err != nil {
			glog.V(1).Infof("unversioned service, ingnoring for svc manifest [%s](%s)", *svc.Name, err)
			continue
		}
		svcdef := serviceDef{
			Version: ver.(string),
		}
		crudsvc, err := reg.GetExtension("venice.apiGrpcCrudService", svc)
		if err != nil {
			glog.V(1).Infof("no crudService specified for [%s](%s)", *svc.Name, err)
		} else {
			glog.V(1).Infof("Found crudsvcs %v", crudsvc.([]string))
			svcdef.Messages = crudsvc.([]string)
		}
		if len(svcdef.Messages) > 0 {
			pkgdef.Svcs[*svc.Name] = svcdef
		}
	}
	if len(pkgdef.Svcs) > 0 {
		manifest[pkg] = pkgdef
	}
	ret, err := json.MarshalIndent(manifest, "", "  ")
	if err != nil {
		glog.Fatalf("failed to marshal service manifest")
	}

	return string(ret), nil
}

type validateArg struct {
	Tpe  string
	Str  string
	Intg uint64
}

type validateFields struct {
	Repeated   bool
	Pointer    bool
	Validators []common.ValidateField
}
type validateMsg struct {
	Fields map[string]validateFields
}

type validators struct {
	Fmap bool
	Map  map[string]validateMsg
}

func checkValidators(file *descriptor.File, msgmap map[string]bool, name string) bool {
	if _, ok := msgmap[name]; ok {
		return msgmap[name]
	}
	m, err := file.Reg.LookupMsg("", name)
	if err != nil {
		glog.Fatalf("Failed to retrieve message %s", name)
	}
	glog.Infof(" checkValidators on %s", name)
	found := false
	// add in map with temp value to handle recursion.
	msgmap[name] = found
	for _, fld := range m.Fields {
		_, err := reg.GetExtension("venice.check", fld)
		if err == nil {
			found = true
		}
		if *fld.Type == gogoproto.FieldDescriptorProto_TYPE_MESSAGE {
			found = found || checkValidators(file, msgmap, *fld.TypeName)
		}
	}
	msgmap[name] = found
	if found {
		glog.Infof(" checkValidators found true %s", name)
	}
	return found
}

func getValidatorManifest(file *descriptor.File) (validators, error) {
	ret := validators{Map: make(map[string]validateMsg)}
	msgmap := make(map[string]bool)
	// freg := file.Reg
	for _, msg := range file.Messages {
		for _, fld := range msg.Fields {
			r, err := reg.GetExtension("venice.check", fld)
			glog.Infof(" Check validator for message %s.%s", *msg.Name, *fld.Name)

			if err == nil {
				glog.Infof(" Found validator for message %s.%s,[ %v ]", *msg.Name, *fld.Name, r)
				if *fld.Type == gogoproto.FieldDescriptorProto_TYPE_MESSAGE {
					return ret, fmt.Errorf("Validators allowed on scalar types only [%s]", *fld.Name)
				}
				if _, ok := ret.Map[*msg.Name]; !ok {
					ret.Map[*msg.Name] = validateMsg{Fields: make(map[string]validateFields)}
				}
				repeated := false
				if *fld.Label == gogoproto.FieldDescriptorProto_LABEL_REPEATED {
					repeated = true
				}
				ret.Map[*msg.Name].Fields[*fld.Name] = validateFields{Repeated: repeated}

				for _, v := range r.([]string) {
					fldv, err := common.ParseValidator(v)
					if err != nil {
						return ret, err
					}
					vfld := ret.Map[*msg.Name].Fields[*fld.Name]
					vfld.Validators = append(vfld.Validators, fldv)
					ret.Map[*msg.Name].Fields[*fld.Name] = vfld
				}

				ret.Fmap = true
			} else {
				glog.Infof("Failed %s", err)
			}
			if *fld.Type == gogoproto.FieldDescriptorProto_TYPE_MESSAGE {
				glog.Infof("checking validators for nested message (%s).%s", *msg.Name, *fld.Name)
				if _, ok := msgmap[*fld.TypeName]; !ok {
					msgmap[*fld.TypeName] = checkValidators(file, msgmap, *fld.TypeName)
				}
				msgname := *msg.Name
				if isNestedMessage(msg) {
					if msg.GetOneofDecl() != nil {
						continue
					}
					msgname, _ = getNestedMsgName(msg)
				}
				if msgmap[*fld.TypeName] == true {
					if _, ok := ret.Map[*msg.Name]; !ok {
						ret.Map[msgname] = validateMsg{Fields: make(map[string]validateFields)}
					}
					if _, ok := ret.Map[*fld.TypeName]; !ok {
						repeated := false
						pointer := true
						if *fld.Label == gogoproto.FieldDescriptorProto_LABEL_REPEATED {
							repeated = true
						}
						if r, err := reg.GetExtension("gogoproto.nullable", fld); err == nil {
							glog.Infof("setting pointer found nullable [%v] for %s]", r, msgname+"/"+*fld.Name)
							pointer = r.(bool)
						} else {
						}
						glog.Infof("setting pointer to [%v] for {%s]", pointer, msgname+"/"+*fld.Name)
						// if it is a embedded field, do not use field name rather use type
						if fld.Embedded {
							// fld.GetTypeName() -> e.g. ".events.EventAttributes"
							temp := strings.Split(fld.GetTypeName(), ".")
							fldType := temp[len(temp)-1]
							ret.Map[msgname].Fields[fldType] = validateFields{Validators: make([]common.ValidateField, 0), Repeated: repeated, Pointer: pointer}
						} else {
							ret.Map[msgname].Fields[*fld.Name] = validateFields{Validators: make([]common.ValidateField, 0), Repeated: repeated, Pointer: pointer}
						}
					}
				}
			}
		}
	}
	glog.Infof("Validator Manifest is %+v", ret)
	return ret, nil
}

// VerDefaults is defaults for a field by version
type VerDefaults struct {
	Repeated bool
	Pointer  bool
	Nested   bool
	Val      string
}

type msgDefaults struct {
	Fields   map[string]common.Defaults
	Versions map[string]map[string]VerDefaults
}

type fileDefaults struct {
	Fmap bool
	Map  map[string]msgDefaults
}

func checkDefaults(file *descriptor.File, msgmap map[string]bool, name string, ret *fileDefaults) (bool, error) {
	if _, ok := msgmap[name]; ok {
		return msgmap[name], nil
	}
	defaulted := false
	msg, err := file.Reg.LookupMsg("", name)
	if err != nil {
		// must be a internal type generated like map entry
		glog.Infof("Failed to retrieve message %s", name)
		return false, nil
	}
	if msg.File != file {
		glog.V(1).Infof("from a different file [%s]", *msg.File.Name)
		if strings.HasPrefix(*msg.File.Name, "github.com/pensando/sw/api/protos/") {
			return true, nil
		}
		return false, nil
	}
	for _, fld := range msg.Fields {
		r, found, err := common.ParseDefaults(file, fld)
		if err != nil {
			glog.V(1).Infof("[%v.%v]got error parsing defaulters (%s)", *msg.Name, *fld.Name, err)
			return false, err
		}
		if found {
			if _, ok := ret.Map[*msg.Name]; !ok {
				glog.V(1).Infof("Creating a new map for [%s]", *msg.Name)
				ret.Map[*msg.Name] = msgDefaults{
					Fields:   make(map[string]common.Defaults),
					Versions: make(map[string]map[string]VerDefaults),
				}
			} else {
				glog.V(1).Infof("Found Existing map for [%s][%+v]", *msg.Name, ret.Map[*msg.Name])
			}

			fldName := *fld.Name
			if fld.Embedded {
				temp := strings.Split(fld.GetTypeName(), ".")
				fldName = temp[len(temp)-1]
			}
			ret.Map[*msg.Name].Fields[fldName] = r
			for fv, fd := range r.Map {
				glog.V(1).Infof("RetMap is [%s][%+v]", *msg.Name, ret.Map[*msg.Name])
				if _, ok := ret.Map[*msg.Name].Versions[fv]; !ok {
					ret.Map[*msg.Name].Versions[fv] = make(map[string]VerDefaults)
				}
				verDef := VerDefaults{
					Repeated: r.Repeated,
					Pointer:  r.Pointer,
					Nested:   r.Nested,
					Val:      fd,
				}
				ret.Map[*msg.Name].Versions[fv][fldName] = verDef
			}
			defaulted = true
		}
		if *fld.Type == gogoproto.FieldDescriptorProto_TYPE_MESSAGE {
			found, err = checkDefaults(file, msgmap, *fld.TypeName, ret)
			if found {
				defaulted = true
				repeated := false
				pointer := true
				if _, ok := ret.Map[*msg.Name]; !ok {
					ret.Map[*msg.Name] = msgDefaults{Fields: make(map[string]common.Defaults), Versions: make(map[string]map[string]VerDefaults)}
				}
				fldName := *fld.Name
				if fld.Embedded {
					temp := strings.Split(fld.GetTypeName(), ".")
					fldName = temp[len(temp)-1]
				}
				if *fld.Label == gogoproto.FieldDescriptorProto_LABEL_REPEATED {
					repeated = true
				}
				if r, err := reg.GetExtension("gogoproto.nullable", fld); err == nil {
					glog.Infof("setting pointer found nullable [%v] for %s]", r, *msg.Name+"/"+*fld.Name)
					pointer = r.(bool)
				}

				ret.Map[*msg.Name].Fields[fldName] = common.Defaults{Pointer: pointer, Repeated: repeated, Nested: true}
			}
		}
	}

	return defaulted, nil
}

func getDefaulterManifest(file *descriptor.File) (fileDefaults, error) {
	ret := fileDefaults{Map: make(map[string]msgDefaults)}
	msgmap := make(map[string]bool)
	for _, msg := range file.Messages {
		var found bool
		var err error
		name := "." + file.GoPkg.Name + "." + *msg.Name
		if found, err = checkDefaults(file, msgmap, name, &ret); err != nil {
			return ret, err
		}
		ret.Fmap = ret.Fmap || found
	}
	return ret, nil
}

// -- Storage Transformers ---
type storageTransformerArgs func(string) bool

// storageTransformerArgMap contains storage transformers recognized by the
// parser. The key is the name of the transformer (as it appears in the
// venice.StorageTransformer="NAME(Args)" annotation) and the value is a list of
// validators, one for each expected argument.
// Currently the only available transformer is "Secret()" (see sw/docs/secrets.md)
//
// Storage transformers must implement the interface defined in
// sw/venice/utils/transformers/storage/types.go and the implementation
// should be placed in the same directory.
// Autogenerated code tries to instantiate a storage transformer called
// <NAME> by invoking the factory function New<NAME>ValueTransformer()
//
// See the setion named "storage transformers" in docs/apidef.md for more details.
var storageTransformerArgMap = map[string][]storageTransformerArgs{
	"Secret": {},
}

type storageTransformerField struct {
	Fn   string
	Args []string
}

type storageTransformerFields struct {
	Repeated     bool
	Pointer      bool
	TypeCast     string
	Transformers []storageTransformerField
}
type storageTransformerMsg struct {
	Fields          map[string]storageTransformerFields
	HasTransformers bool // true if this message or any nested message has fields with storageTransformers
}

type storageTransformers struct {
	Fmap bool
	Map  map[string]storageTransformerMsg
}

func checkStorageTransformers(file *descriptor.File, msgmap map[string]bool, name string) bool {
	if _, ok := msgmap[name]; ok {
		return msgmap[name]
	}
	m, err := file.Reg.LookupMsg("", name)
	if err != nil {
		glog.Fatalf("Failed to retrieve message %s", name)
	}
	glog.Infof(" checkStorageTransformers on %s", name)
	found := false
	// add in map with temp value to handle recursion.
	msgmap[name] = found
	for _, fld := range m.Fields {
		_, err := reg.GetExtension("venice.storageTransformer", fld)
		if err == nil {
			found = true
		}
		if *fld.Type == gogoproto.FieldDescriptorProto_TYPE_MESSAGE {
			found = found || checkStorageTransformers(file, msgmap, *fld.TypeName)
		}
	}
	msgmap[name] = found
	if found {
		glog.Infof(" checkStorageTransformers found true %s", name)
	}
	return found
}

func parseStorageTransformers(in string) (*storageTransformerField, error) {
	ret := storageTransformerField{}
	re := regexp.MustCompile("^(?P<func>[a-zA-Z0-9_\\-]+)\\((?P<args>[a-zA-Z0-9_\\-\\, \\.\\:]*)*\\)")
	params := re.FindStringSubmatch(in)
	if params == nil {
		return nil, fmt.Errorf("Failed to parse storageTransformer [%s]", in)
	}
	ret.Fn = params[1]
	ret.Args = strings.Split(strings.Replace(params[2], " ", "", -1), ",")
	if len(ret.Args) == 1 && ret.Args[0] == params[2] && params[2] == "" {
		ret.Args = []string{}
	}
	if targs, ok := storageTransformerArgMap[ret.Fn]; ok {
		if len(targs) != len(ret.Args) {
			return nil, fmt.Errorf("Incorrect number of args (%d) for %s", len(ret.Args), ret.Fn)
		}
		for i := range targs {
			if !targs[i](ret.Args[i]) {
				return nil, fmt.Errorf("storageTransformer validation for arg(%s) failed for %s", ret.Args[i], ret.Fn)
			}
		}
	} else {
		return nil, fmt.Errorf("unknown storageTransformer %s", ret.Fn)
	}
	return &ret, nil
}

func getStorageTransformersManifest(file *descriptor.File) (*storageTransformers, error) {
	ret := storageTransformers{Map: make(map[string]storageTransformerMsg)}
	msgmap := make(map[string]bool)
	for _, msg := range file.Messages {
		for _, fld := range msg.Fields {
			r, err := reg.GetExtension("venice.storageTransformer", fld)
			glog.Infof(" Check storageTransformer for message %s.%s", *msg.Name, *fld.Name)

			if err == nil {
				glog.Infof(" Found storageTransformer for message %s.%s,[ %v ]", *msg.Name, *fld.Name, r)
				if *fld.Type == gogoproto.FieldDescriptorProto_TYPE_MESSAGE {
					return nil, fmt.Errorf("StorageTransformers allowed on scalar types only [%s]", *fld.Name)
				}
				if *fld.Type != gogoproto.FieldDescriptorProto_TYPE_STRING &&
					*fld.Type != gogoproto.FieldDescriptorProto_TYPE_BYTES {
					return nil, fmt.Errorf("StorageTransformers allowed on types \"strings\" and \"bytes\" only [%s]", *fld.Name)
				}
				if _, ok := ret.Map[*msg.Name]; !ok {
					ret.Map[*msg.Name] = storageTransformerMsg{
						Fields:          make(map[string]storageTransformerFields),
						HasTransformers: true,
					}
				}
				repeated := false
				if *fld.Label == gogoproto.FieldDescriptorProto_LABEL_REPEATED {
					repeated = true
				}
				typeCast := ""
				if fld.IsString() {
					typeCast = "string"
				} else if fld.IsBytes() {
					typeCast = "[]byte"
				} else {
					return nil, fmt.Errorf("Transformers allowed on string and bytes types only [%s]", *fld.Name)
				}
				ret.Map[*msg.Name].Fields[*fld.Name] = storageTransformerFields{
					Repeated: repeated,
					TypeCast: typeCast,
				}
				for _, v := range r.([]string) {
					fldv, err := parseStorageTransformers(v)
					if err != nil {
						return nil, err
					}
					tfld := ret.Map[*msg.Name].Fields[*fld.Name]
					tfld.Transformers = append(tfld.Transformers, *fldv)
					ret.Map[*msg.Name].Fields[*fld.Name] = tfld
				}

				ret.Fmap = true
			} else {
				glog.Infof("Failed %s", err)
			}
			if *fld.Type == gogoproto.FieldDescriptorProto_TYPE_MESSAGE {
				glog.Infof("checking storageTransformer for nested message (%s).%s", *msg.Name, *fld.Name)
				if _, ok := msgmap[*fld.TypeName]; !ok {
					msgmap[*fld.TypeName] = checkStorageTransformers(file, msgmap, *fld.TypeName)
				}
				msgname := *msg.Name
				if isNestedMessage(msg) {
					msgname, _ = getNestedMsgName(msg)
				}
				if msgmap[*fld.TypeName] == true {
					if _, ok := ret.Map[*msg.Name]; !ok {
						ret.Map[msgname] = storageTransformerMsg{
							Fields:          make(map[string]storageTransformerFields),
							HasTransformers: true,
						}
					}
					if _, ok := ret.Map[*fld.Name]; !ok {
						repeated := false
						pointer := true
						if *fld.Label == gogoproto.FieldDescriptorProto_LABEL_REPEATED {
							repeated = true
						}
						if r, err := reg.GetExtension("gogoproto.nullable", fld); err == nil {
							glog.Infof("setting pointer found nullable [%v] for %s]", r, msgname+"/"+*fld.Name)
							pointer = r.(bool)
						} else {
						}
						glog.Infof("setting pointer to [%v] for {%s]", pointer, msgname+"/"+*fld.Name)
						// if it is a embedded field, do not use field name rather use type
						if fld.Embedded {
							// fld.GetTypeName() -> e.g. ".events.EventAttributes"
							temp := strings.Split(fld.GetTypeName(), ".")
							fldType := temp[len(temp)-1]
							ret.Map[msgname].Fields[fldType] = storageTransformerFields{Transformers: make([]storageTransformerField, 0), Repeated: repeated, Pointer: pointer}
						} else {
							ret.Map[msgname].Fields[*fld.Name] = storageTransformerFields{Transformers: make([]storageTransformerField, 0), Repeated: repeated, Pointer: pointer}
						}
					}
				}
			}
		}
	}
	glog.Infof("StorageTransformers Manifest is %+v", ret.Map)
	return &ret, nil
}

// --- End Storage Transformers ---

func derefStr(in *string) string {
	return *in
}

func getEnumStrMap(file *descriptor.File, in []string) (string, error) {
	return common.GetEnumStr(file, in, "value")
}

// relationRef is reference to relations
type relationRef struct {
	Type  string
	To    string
	Field string
}

var relMap = make(map[string][]relationRef)

func addRelations(f *descriptor.Field) error {
	if r, err := reg.GetExtension("venice.objRelation", f); err == nil {
		glog.V(1).Infof("Checking relation to %s", *f.Name)
		name := f.Message.File.GoPkg.Name + "." + *f.Message.Name
		if fr, ok := r.(venice.ObjectRln); ok {
			glog.V(1).Infof("adding relation to %s.%s", name, *f.Name)
			m := relationRef{Type: fr.Type, To: fr.To, Field: *f.Name}
			relMap[name] = append(relMap[name], m)
		}
	} else {
		glog.V(1).Infof("relations.. not found on %s", *f.Name)
	}
	return nil
}

func genRelMap(path string) (string, error) {
	if _, err := os.Stat(path); os.IsNotExist(err) {
		glog.V(1).Infof("relationRef [%s] not found", path)
	} else {
		glog.V(1).Infof("relationRef exists, reading from manifest")
		raw, err := ioutil.ReadFile(path)
		if err != nil {
			glog.V(1).Infof("Reading Relation failed (%s)", err)
			return "", err
		}
		rmap := make(map[string][]relationRef)
		err = json.Unmarshal(raw, &rmap)
		if err != nil {
			glog.V(1).Infof("Json Unmarshall of rel map file failed ignoring current file")
		} else {
			for k, v := range rmap {
				if _, ok := relMap[k]; ok {
					relMap[k] = append(relMap[k], v...)
				} else {
					relMap[k] = v
				}
			}
		}
	}
	if len(relMap) > 0 {
		ret, err := json.MarshalIndent(relMap, "", "  ")
		if err != nil {
			glog.V(1).Infof("Failed to marshall output rel map")
			return "", err
		}
		str := string(ret[:])
		glog.V(1).Infof("Generated Relations.json %v", str)
		return str, nil
	}
	return "{}", nil
}

// Atlease 2 character long.
func detitle(str string) (string, error) {
	if len(str) > 1 {
		ret := strings.ToLower(string(str[0])) + string(str[1:])
		return ret, nil
	}
	return str, errors.New("too short")
}

func getInputType(pkg string, method *descriptor.Method, fq bool) string {
	if method.RequestType.File.GoPkg.Name == pkg {
		glog.V(1).Infof("Evaluating is same %s [%s]", pkg, method.RequestType.File.GoPkg.Name)
		if fq {
			return fmt.Sprintf("%s.%s", pkg, *method.RequestType.Name)
		}
		return *method.RequestType.Name
	}
	r := method.InputType
	ret := strings.TrimPrefix(*r, ".")
	return ret
}

func getOutputType(pkg string, method *descriptor.Method, fq bool) string {
	if method.ResponseType.File.GoPkg.Name == pkg {
		if fq {
			return fmt.Sprintf("%s.%s", pkg, *method.ResponseType.Name)
		}
		return *method.ResponseType.Name
	}
	r := method.OutputType
	ret := strings.TrimPrefix(*r, ".")
	return ret
}

func getListType(msg *descriptor.Message, fq bool) (string, error) {
	for _, f := range msg.Fields {
		if *f.Name == "Items" {
			if fq {
				return strings.TrimPrefix(*f.TypeName, "."), nil
			}
			parts := strings.Split(*f.TypeName, ".")
			return parts[len(parts)-1], nil
		}
	}
	return "", errors.New("list item not found")
}

func getListTypeMsg(msg *descriptor.Message) (*descriptor.Message, error) {
	for _, f := range msg.Fields {
		if *f.Name == "Items" {
			ret, err := msg.File.Reg.LookupMsg("", f.GetTypeName())
			if err == nil {
				return ret, nil
			}
			return nil, err
		}
	}
	return nil, errors.New("list item not found")
}

func getWatchType(msg *descriptor.Message, fq bool) (string, error) {
	for _, f := range msg.Fields {
		if *f.Name == "Events" {
			nmsg, err := msg.File.Reg.LookupMsg("", f.GetTypeName())
			if err != nil {
				return "", errors.New("Object type not found")
			}
			for _, nf := range nmsg.Fields {
				if *nf.Name == "Object" {
					if fq {
						return strings.TrimPrefix(nf.GetTypeName(), "."), nil
					}
					parts := strings.Split(nf.GetTypeName(), ".")
					return parts[len(parts)-1], nil
				}
			}
		}
	}
	return "", errors.New("Object item not found")
}

func getWatchTypeMsg(msg *descriptor.Message) (*descriptor.Message, error) {
	for _, f := range msg.Fields {
		if *f.Name == "Events" {
			nmsg, err := msg.File.Reg.LookupMsg("", f.GetTypeName())
			if err != nil {
				return nil, errors.New("Object type not found")
			}
			for _, nf := range nmsg.Fields {
				if *nf.Name == "Object" {
					ret, err := msg.File.Reg.LookupMsg("", nf.GetTypeName())
					if err == nil {
						return ret, nil
					}
					return nil, err
				}
			}
		}
	}
	return nil, errors.New("Object item not found")
}

func isListHelper(msg *descriptor.Message) bool {
	v, err := reg.GetExtension("venice.objectAutoGen", msg)
	if err == nil {
		glog.V(1).Infof("Found Extension %s", v.(string))
		return (v.(string) == "listhelper")
	}
	glog.V(1).Infof("Extension not found (%s)", err)

	return false
}

func isWatchHelper(msg *descriptor.Message) bool {
	if v, err := reg.GetExtension("venice.objectAutoGen", msg); err == nil {
		return (v.(string) == "watchhelper")
	}
	return false
}

func isAutoList(meth *descriptor.Method) bool {
	if v, err := reg.GetExtension("venice.methodAutoGen", meth); err == nil {
		if v.(bool) == false {
			return false
		}
		if v1, err := reg.GetExtension("venice.methodOper", meth); err == nil {
			if v1.(string) == "list" {
				return true
			}
		}
	}
	return false
}

func isAutoWatch(meth *descriptor.Method) bool {
	if v, err := reg.GetExtension("venice.methodAutoGen", meth); err == nil {
		if v.(bool) == false {
			return false
		}
		if v1, err := reg.GetExtension("venice.methodOper", meth); err == nil {
			if v1.(string) == "watch" {
				return true
			}
		}
	}
	return false
}

func getPackageCrudObjects(file *descriptor.File) ([]string, error) {
	var crudmap = make(map[string]bool)
	for _, svc := range file.Services {
		cruds, err := reg.GetExtension("venice.apiGrpcCrudService", svc)
		if err == nil {
			for _, v := range cruds.([]string) {
				crudmap[v] = true
			}
		}
	}
	var ret []string
	for k := range crudmap {
		ret = append(ret, k)
	}
	sort.Strings(ret)
	return ret, nil
}

// ActionEndpoints specifies parameters of an action
type ActionEndpoints struct {
	Name              string
	Request, Response string
}

func getSvcCrudObjects(svc *descriptor.Service) ([]string, error) {
	var ret []string
	cruds, err := reg.GetExtension("venice.apiGrpcCrudService", svc)
	if err == nil {
		ret = cruds.([]string)
	}
	return ret, nil
}

func getSvcActionEndpoints(svc *descriptor.Service, target string) ([]ActionEndpoints, error) {
	var ret []ActionEndpoints
	act, err := reg.GetExtension("venice.apiAction", svc)
	if err != nil {
		return ret, nil
	}
	for _, r := range act.([]*venice.ActionEndpoint) {
		tgt := ""
		if tgt = r.GetCollection(); tgt == "" {
			if tgt = r.GetObject(); tgt == "" {
				continue
			}
		}
		if tgt == target {
			obj := ActionEndpoints{Name: strings.Title(r.Action), Request: r.Request, Response: r.Response}
			ret = append(ret, obj)
		}
	}
	return ret, nil
}

func isRestExposed(meth *descriptor.Method) bool {
	glog.V(1).Infof("Checking for rest exposed for %s\n", *meth.Name)
	if _, err := reg.GetExtension("google.api.http", meth); err == nil {
		return true
	}
	return false
}

func isRestMethod(svc *descriptor.Service, oper, object string) bool {
	method := oper + object
	for _, v := range svc.Methods {
		if *v.Name == method {
			return isRestExposed(v)
		}
	}
	return false
}

// isMapEntry checks if the message is a auto generated map entry message
func isMapEntry(msg *descriptor.Message) bool {
	glog.V(1).Infof("Looking for mapEntry in %s)", *msg.Name)
	if opt := msg.GetOptions(); opt != nil {
		return opt.GetMapEntry()
	}
	return false
}

func isNestedMessage(msg *descriptor.Message) bool {
	glog.V(1).Infof("Check nested message %s[%v]", *msg.Name, msg.Outers)
	if len(msg.Outers) != 0 {
		return true
	}
	return false
}

func getNestedMsgName(msg *descriptor.Message) (string, error) {
	if len(msg.Outers) == 0 {
		return "", errors.New("not a nested message")
	}
	ret := ""
	dlmtr := ""
	for _, n := range msg.Outers {
		ret = ret + dlmtr + n
		dlmtr = "_"
	}
	return ret + "_" + *msg.Name, nil
}

func isSpecStatusMessage(msg *descriptor.Message) bool {
	glog.V(1).Infof("Check if SpecStatus message for %s", *msg.Name)
	spec := false
	status := false
	for _, v := range msg.Fields {
		if *v.Name == "Spec" {
			spec = true
		}
		if *v.Name == "Status" {
			status = true
		}
	}

	return spec && status
}

func getAutoRestOper(meth *descriptor.Method) (string, error) {
	if v, err := reg.GetExtension("venice.methodAutoGen", meth); err == nil {
		if v.(bool) == false {
			return "", errors.New("not an autogen method")
		}
		if v1, err := reg.GetExtension("venice.methodOper", meth); err == nil {
			switch v1.(string) {
			case "create":
				return "POST", nil
			case "update":
				return "PUT", nil
			case "get", "list":
				return "GET", nil
			case "delete":
				return "DELETE", nil
			case "watch":
				return "", errors.New("not supported method")
			default:
				return "", errors.New("not supported method")
			}
		}
	}
	return "", errors.New("not an autogen method")
}

func getJSONTag(fld *descriptor.Field) string {
	if r, err := reg.GetExtension("gogoproto.jsontag", fld); err == nil {
		t := strings.Split(r.(string), ",")
		if len(t) > 0 {
			return t[0]
		}
	}
	return ""
}

// Field represents the schema details of a field
type Field struct {
	Name       string
	JSONTag    string
	CLITag     cliInfo
	Pointer    bool
	Inline     bool
	FromInline bool
	Slice      bool
	Map        bool
	// KeyType is valid only when Map is true
	KeyType string
	Type    string
}

// cliInfo captures all the parameters related to CLI
type cliInfo struct {
	tag  string
	path string
	ins  string
	skip bool
	help string
}

// Struct represents the schema details of a field
type Struct struct {
	Kind     string
	APIGroup string
	CLITags  map[string]cliInfo
	Fields   map[string]Field
	// keys is used to keep a stable order of Fields when generating the schema. This is
	//  a ordered set of keys in the Fielda map and follows the order in the corresponding
	//  slice in DescriptorProto.
	keys     []string
	mapEntry bool
}

// cliTagRegex is a regex for validating CLI parameters. Initialized in init.
var cliTagRegex *regexp.Regexp

// list of valid CLI tags
const (
	CLISSkipTag  = "verbose-only"
	CLIInsertTag = "ins"
	CLIIdTag     = "id"
)

// Validate string CLI tags
func validateCLITag(in string) bool {
	return cliTagRegex.MatchString(in)
}

// parseCLITags parses the cli-tags: string and updates the passed in Field
func parseCLITags(in string, fld *Field) {
	in = strings.TrimSpace(in)
	in = strings.TrimPrefix(in, "cli-tags:")
	fields := strings.Fields(in)
	for _, f := range fields {
		kv := strings.Split(f, "=")
		if len(kv) != 2 {
			glog.Fatalf("Invalid CLI tag specification [%v]", f)
		}
		switch kv[0] {
		case CLISSkipTag:
			b, err := strconv.ParseBool(kv[1])
			if err != nil {
				glog.Fatalf("Invalid format for CLI tag for %s [%v]", CLISSkipTag, f)
			}
			fld.CLITag.skip = b
		case CLIInsertTag:
			if !validateCLITag(kv[1]) {
				glog.Fatalf("Invalid format for CLI tag [%v]", f)
			}
			fld.CLITag.ins = kv[1]
		case CLIIdTag:
			if !validateCLITag(kv[1]) {
				glog.Fatalf("Invalid format for CLI tag [%v]", f)
			}
			fld.CLITag.tag = kv[1]
		}
	}
	if fld.CLITag.ins != "" {
		fld.CLITag.tag = fld.CLITag.ins + "-" + fld.CLITag.tag
	}
}

// parseMessageCLIParams parses and updates CLI tags and help strings for a message
func parseMessageCLIParams(strct *Struct, msg *descriptor.Message, path string, locs []int, file *descriptor.File, msgMap map[string]Struct) {
	for flid, fld := range msg.Fields {
		fpath := append(locs, common.FieldType, flid)
		loc, err := common.GetLocation(file.SourceCodeInfo, fpath)
		if err != nil {
			continue
		}
		name := *fld.Name
		if common.IsInline(fld) {
			p := strings.Split(*fld.TypeName, ".")
			name = p[len(p)-1]
		}
		sfld, ok := strct.Fields[name]
		if !ok {
			glog.Fatalf("did not find struct field for %s.%s", path, name)
		}
		if err != nil {
			continue
		}
		for _, line := range strings.Split(loc.GetLeadingComments(), "\n") {
			line = strings.TrimSpace(line)
			if strings.HasPrefix(line, "cli-tags:") {
				parseCLITags(line, &sfld)
			}
			if strings.HasPrefix(line, "cli-help:") {
				sfld.CLITag.help = strings.TrimSpace(strings.TrimPrefix(line, "cli-help:"))
			}
		}
		for nid, nmsg := range msg.NestedType {
			nfqname := path + "." + *nmsg.Name
			nstrct, ok := msgMap[nfqname]
			if !ok {
				glog.Fatalf("did not find struct for %s", nfqname)
			}
			nestedMsg, err := file.Reg.LookupMsg("", nfqname)
			if err != nil {
				glog.Fatalf("Could not find nested message %v (%s)", *nmsg.Name, err)
			}
			nlocs := append(locs, common.NestedMsgType, nid)
			parseMessageCLIParams(&nstrct, nestedMsg, nfqname, nlocs, file, msgMap)
			msgMap[nfqname] = nstrct
		}
		strct.Fields[*fld.Name] = sfld
	}
}

// getCLIParams parses and updates CLI tags and help strings for a file.
func getCLIParams(file *descriptor.File, msgMap map[string]Struct) {
	pkg := file.GoPkg.Name
	for id, msg := range file.Messages {
		fqname := pkg + "." + *msg.Name
		if len(msg.Outers) > 0 {
			fqname = msg.Outers[0] + "." + *msg.Name
			fqname = strings.TrimPrefix(fqname, ".")
			fqname = pkg + "." + fqname
		}
		strct, ok := msgMap[fqname]
		if !ok {
			glog.Fatalf("did not find struct for %s", fqname)
		}
		locs := []int{common.MsgType, id}
		parseMessageCLIParams(&strct, msg, fqname, locs, file, msgMap)
		msgMap[fqname] = strct
	}
}

func genField(msg string, fld *descriptor.Field, file *descriptor.File) (Field, error) {
	ret := Field{}
	repeated := false
	if fld.Label != nil && *fld.Label == gogoproto.FieldDescriptorProto_LABEL_REPEATED {
		repeated = true
	}
	pointer := true
	if r, err := reg.GetExtension("gogoproto.nullable", fld); err == nil {
		glog.Infof("setting pointer found nullable [%v] for %s]", r, msg+"/"+*fld.Name)
		pointer = r.(bool)
	}
	inline := common.IsInline(fld)
	isMap := false
	typeName := ""
	keyType := ""
	if *fld.Type == gogoproto.FieldDescriptorProto_TYPE_MESSAGE {
		typeName = fld.GetTypeName()
		fmsg, err := file.Reg.LookupMsg("", typeName)
		if err != nil {
			glog.Infof("failed to get field %s", *fld.Name)
			return ret, fmt.Errorf("failed to get field %s", *fld.Name)
		}
		if isMapEntry(fmsg) {
			isMap = true
			repeated = false
			for _, v := range fmsg.Fields {
				if *v.Name == "value" {
					if v.GetType() == gogoproto.FieldDescriptorProto_TYPE_MESSAGE {
						typeName = v.GetTypeName()
						typeName = strings.TrimPrefix(typeName, ".")
					} else {
						typeName = gogoproto.FieldDescriptorProto_Type_name[int32(v.GetType())]
					}
				}
				if *v.Name == "key" {
					if v.GetType() == gogoproto.FieldDescriptorProto_TYPE_MESSAGE {
						keyType = v.GetTypeName()
						keyType = strings.TrimPrefix(keyType, ".")
					} else {
						keyType = gogoproto.FieldDescriptorProto_Type_name[int32(v.GetType())]
					}
				}
			}
		} else {
			typeName = strings.TrimPrefix(typeName, ".")
		}
	} else {
		typeName = gogoproto.FieldDescriptorProto_Type_name[int32(fld.GetType())]
	}
	// TODO: Handle other cases for CLI tag resolution
	cliTag := common.GetJSONTag(fld)
	if cliTag == "" {
		cliTag = *fld.Name
	}
	name := *fld.Name
	if inline {
		p := strings.Split(*fld.TypeName, ".")
		name = p[len(p)-1]
	}
	ret = Field{
		Name:    name,
		JSONTag: common.GetJSONTag(fld),
		CLITag:  cliInfo{tag: cliTag},
		Pointer: pointer,
		Slice:   repeated,
		Map:     isMap,
		Inline:  inline,
		KeyType: keyType,
		Type:    typeName,
	}
	return ret, nil
}

// isScalarType returns true for all protobuf scalars
func isScalarType(in string) bool {
	switch in {
	case "TYPE_FLOAT", "TYPE_DOUBLE", "TYPE_INT64", "TYPE_UINT64", "TYPE_INT32", "TYPE_FIXED64", "TYPE_FIXED32", "TYPE_BOOL":
		return true
	case "TYPE_STRING", "TYPE_BYTES", "TYPE_UINT32", "TYPE_ENUM", "TYPE_SFIXED32", "TYPE_SFIXED64", "TYPE_SINT32", "TYPE_SINT64":
		return true
	}
	return false
}

// getCLITags updates the CLITags for the give Struct. It recurses through all fields and their
//   types in the message.
func getCLITags(strct Struct, path string, msgMap map[string]Struct, m map[string]cliInfo) error {
	if path != "" {
		path = path + "."
	}
	for _, k := range strct.keys {
		fld := strct.Fields[k]
		if isScalarType(fld.Type) {
			fpath := path + fld.Name
			if _, ok := m[fld.CLITag.tag]; ok {
				// panic(fmt.Sprintf("duplicate tag [%s] at [%s]", fld.CLITag, fpath))
				// Dont panic during initial development. Will panic in production
				glog.V(1).Infof("Duplicate tag [%v] at [%s] Will CRASH&BURN", fld.CLITag, fpath)
			}
			fld.CLITag.path = fpath
			m[fld.CLITag.tag] = fld.CLITag
			continue
		}
		fpath := path + fld.Name
		if fld.Map || fld.Slice {
			fpath = fpath + "[]"
		}
		getCLITags(msgMap[fld.Type], fpath, msgMap, m)
	}
	return nil
}

// flattenInline flattens any inlined structures in to the parent struct.
// XXX-TODO(sanjayt): Carry CLI Tags from the inlined elements to the flattened members.
func flattenInline(file *descriptor.File, field Field) (map[string]Field, []string, error) {
	glog.V(1).Infof("Generating Inline for field %v/%v", field.Type, field.Name)
	ret := make(map[string]Field)
	keys := []string{}
	msg, err := file.Reg.LookupMsg("", field.Type)
	if err != nil {
		return ret, keys, err
	}
	glog.V(1).Infof("Got Message %v", *msg.Name)
	for _, fld := range msg.Fields {
		f, err := genField("", fld, file)
		if err != nil {
			return ret, keys, err
		}
		f.FromInline = true
		glog.V(1).Infof("Inline Got field %v", f.Name)
		name := f.Name
		if f.Inline {
			p := strings.Split(f.Type, ".")
			name = p[len(p)-1]
		}

		if _, ok := ret[name]; !ok {
			glog.V(1).Infof("Inline adding field %v", f.Name)
			ret[name] = f
			keys = append(keys, name)
		}
		if common.IsInline(fld) {
			flds, ks, err := flattenInline(file, f)
			if err != nil {
				return ret, keys, err
			}
			for _, v := range ks {
				if _, ok := ret[v]; !ok {
					ret[v] = flds[v]
					keys = append(keys, v)
				}
			}
		}
	}
	return ret, keys, nil
}

// genMsgMap parses and generates the schema map for the file.
func genMsgMap(file *descriptor.File) (map[string]Struct, []string, error) {
	pkg := file.GoPkg.Name
	ret := make(map[string]Struct)
	var keys []string
	for _, msg := range file.Messages {
		var kind, group string
		if isSpecStatusMessage(msg) {
			kind = *msg.Name
			group = file.GoPkg.Name
		}
		fqname := pkg + "." + *msg.Name
		if len(msg.Outers) > 0 {
			fqname = msg.Outers[0] + "." + *msg.Name
			fqname = strings.TrimPrefix(fqname, ".")
			fqname = pkg + "." + fqname
		}
		node := Struct{Kind: kind, APIGroup: group, CLITags: make(map[string]cliInfo), Fields: make(map[string]Field), mapEntry: isMapEntry(msg)}
		for _, fld := range msg.Fields {
			f, err := genField(fqname, fld, file)
			if err != nil {
				return ret, keys, err
			}
			name := f.Name
			if f.Inline {
				p := strings.Split(f.Type, ".")
				name = p[len(p)-1]
			}
			node.Fields[name] = f
			node.keys = append(node.keys, name)
			if f.Inline {
				// Flatten the fields that are inlined and add them here.
				flds, names, err := flattenInline(file, f)
				if err == nil {
					for _, k := range names {
						glog.V(1).Infof("check inline field %v", k)
						if _, ok := node.Fields[k]; !ok {
							glog.V(1).Infof("add inline field %v", k)
							node.Fields[k] = flds[k]
							node.keys = append(node.keys, k)
						}
					}
				} else {
					return ret, keys, err
				}
			}

		}
		ret[fqname] = node
		keys = append(keys, fqname)
	}
	getCLIParams(file, ret)
	for _, msg := range file.Messages {
		if !isSpecStatusMessage(msg) {
			continue
		}
		fqname := pkg + "." + *msg.Name
		strct := ret[fqname]
		getCLITags(strct, "", ret, strct.CLITags)
		ret[fqname] = strct
	}
	return ret, keys, nil
}

// getMsgMap is the template function used by templates to emit the schema for the file.
func getMsgMap(file *descriptor.File) (string, error) {
	msgs, keys, err := genMsgMap(file)
	if err != nil {
		return "", err
	}
	ret := ""
	pkg := "api."
	if file.GoPkg.Name == "api" {
		pkg = ""
	}
	for _, k := range keys {
		s := msgs[k]
		kpaths := strings.Split(k, ".")
		objPath := strings.Join(kpaths[1:], "_")
		if s.mapEntry {
			ret = fmt.Sprintf("%s\n\"%s\": &%vStruct{\n Fields: map[string]%vField {", ret, k, pkg, pkg)
		} else {
			ret = fmt.Sprintf("%s\n\"%s\": &%vStruct{\n Kind: \"%s\", APIGroup: \"%s\", GetTypeFn: func() reflect.Type { return reflect.TypeOf(%s{}) }, \nFields: map[string]%vField {", ret, k, pkg, s.Kind, s.APIGroup, objPath, pkg)
		}

		for _, k1 := range s.keys {
			f := s.Fields[k1]
			ret = ret + fmt.Sprintf("\n\"%s\":%vField{Name: \"%s\", CLITag: %vCLIInfo{Path: \"%s\", Skip: %v, Insert: \"%s\", Help:\"%s\"}, JSONTag: \"%s\", Pointer: %v, Slice:%v, Map:%v, Inline: %v, FromInline: %v, KeyType: \"%v\", Type: \"%s\"},\n",
				f.Name, pkg, f.Name, pkg, f.CLITag.path, f.CLITag.skip, f.CLITag.ins, f.CLITag.help, f.JSONTag, f.Pointer, f.Slice, f.Map, f.Inline, f.FromInline, f.KeyType, f.Type)
		}
		ret = ret + "}, \n"
		if len(s.CLITags) > 0 {
			clikeys := []string{}
			for k1 := range s.CLITags {
				clikeys = append(clikeys, k1)
			}
			sort.Strings(clikeys)
			ret = ret + "\n CLITags: map[string]" + pkg + "CLIInfo { \n"
			for _, v := range clikeys {
				ret = ret + fmt.Sprintf("\"%v\": %vCLIInfo{Path: \"%s\", Skip: %v, Insert: \"%s\", Help:\"%s\"},\n", v, pkg, s.CLITags[v].path, s.CLITags[v].skip, s.CLITags[v].ins, s.CLITags[v].help)
			}
			ret = ret + "}, \n"
		}

		ret = ret + "},"
	}
	return ret, nil
}

func isAPIServerServed(file *descriptor.File) (bool, error) {
	if v, err := reg.GetExtension("venice.fileApiServerBacked", file); err == nil {
		return v.(bool), nil
	}
	return true, nil
}

func getRelPath(file *descriptor.File) (string, error) {
	return "github.com/pensando/sw/api", nil
}

//getGrpcDestination returns the gRPC destination specified.
func getGrpcDestination(file *descriptor.File) string {
	if v, err := reg.GetExtension("venice.fileGrpcDest", file); err == nil {
		return v.(string)
	}
	return ""
}

// getFileName returns a filename sans the extension and path
func getFileName(name string) string {
	return strings.Title(strings.TrimSuffix(filepath.Base(name), filepath.Ext(filepath.Base(name))))
}

// returns the list of event types
func getEventTypes(s *descriptor.Service) ([]string, error) {
	ets := []string{}
	enumNames, err := reg.GetExtension("venice.eventTypes", s)
	if err != nil {
		return ets, nil
	}

	// look through all the enums and construct slice of event types
	if enumNames != nil {
		for _, eName := range enumNames.([]string) {
			eventTypes, err := s.File.Reg.LookupEnum("", fmt.Sprintf(".%s.%s", s.File.GetPackage(), eName))
			if err != nil {
				continue
			}

			// read individual event type
			for _, val := range eventTypes.GetValue() {
				ets = append(ets, val.GetName())
			}
		}
	}

	return ets, nil
}

func getFileCategory(m *descriptor.Message) (string, error) {
	if ext, err := reg.GetExtension("venice.fileCategory", m.File); err == nil {
		return ext.(string), nil
	}
	return globals.ConfigURIPrefix, nil
}

func isStreaming(m *descriptor.Method) (bool, error) {
	return (m.ClientStreaming != nil && *m.ClientStreaming) || (m.ServerStreaming != nil && *m.ServerStreaming), nil
}

func isClientStreaming(m *descriptor.Method) (bool, error) {
	return (m.ClientStreaming != nil && *m.ClientStreaming), nil
}

func isSvcWatch(meth *descriptor.Method, svcName string) bool {
	return meth.GetName() == "AutoWatchSvc"+svcName
}

func getAPIOperType(in string) (string, error) {
	switch in {
	case "CreateOper":
		return "create", nil
	case "UpdateOper":
		return "update", nil
	case "GetOper":
		return "get", nil
	case "DeleteOper":
		return "delete", nil
	case "ListOper":
		return "list", nil
	case "WatchOper":
		return "watch", nil
	default:
		return "unknown", errors.New("unknown oper")
	}
}

//--- Mutators functions ---//
func reqMutator(req *plugin.CodeGeneratorRequest) {
	mutator.AddAutoGrpcEndpoints(req)
}

func getWatchHelperName(in string) string {
	return "AutoMsg" + in + "WatchHelper"
}

func isTenanted(msg *descriptor.Message) (bool, error) {
	dbk, err := getDbKey(msg)
	if err != nil {
		return false, err
	}
	for _, c := range dbk {
		if c.Type == "field" && c.Val == "Tenant" {
			return true, nil
		}
	}
	return false, nil
}

func isObjTenanted(file *descriptor.File, obj string) (bool, error) {
	name := "." + file.GoPkg.Name + "." + obj
	msg, err := file.Reg.LookupMsg("", name)
	if err != nil {
		return false, err
	}
	return isTenanted(msg)
}

// ProxyPath is parameters for reverse proxy endpoints
type ProxyPath struct {
	Prefix   string
	FullPath string
	Backend  string
}

func getProxyPaths(svc *descriptor.Service) ([]ProxyPath, error) {
	var ret []ProxyPath
	svcParams, err := getSvcParams(svc)
	if err != nil {
		glog.V(1).Infof("unable to get proxy paths for service [%s]", *svc.Name)
		return ret, err
	}
	i, err := reg.GetExtension("venice.proxyPrefix", svc)
	if err != nil {
		glog.V(1).Infof("no proxy options found on service [%s](%s)", *svc.Name, err)
		return ret, nil
	}
	opts, ok := i.([]*venice.ProxyEndpoint)
	if !ok {
		glog.V(1).Infof("could not parse option")
		return ret, fmt.Errorf("could not parse proxy option for service [%s] [%+v]", *svc.Name, opts)
	}
	glog.V(1).Infof("found proxy options on service [%s] [%+v]", *svc.Name, opts)
	pathMap := make(map[string]bool)
	category := globals.ConfigURIPrefix
	if i, err = reg.GetExtension("venice.fileCategory", svc.File); err == nil {
		if category, ok = i.(string); !ok {
			category = globals.ConfigURIPrefix
		}
	} else {
		glog.V(1).Infof("Did not find Category %s", err)
	}
	for _, opt := range opts {
		if _, ok := pathMap[opt.GetPathPrefix()]; ok {
			glog.Fatalf("duplicate path detected in proxy paths service [%s] path [%s]", *svc.Name, opt.GetPathPrefix())
		}

		path := "/" + category + "/" + svcParams.Prefix + "/" + svcParams.Version + "/" + strings.TrimPrefix(opt.GetPathPrefix(), "/")
		ret = append(ret, ProxyPath{Prefix: opt.GetPathPrefix(), FullPath: path, Backend: opt.GetBackend()})
	}
	return ret, nil
}

func init() {
	cliTagRegex = regexp.MustCompile(`^[a-zA-Z0-9_\-]+$`)

	// Register Option Parsers
	common.RegisterOptionParsers()

	// Register Functions
	reg.RegisterFunc("getDbKey", getDbKey)
	reg.RegisterFunc("getURIKey", getURIKey)
	reg.RegisterFunc("getMsgURIKey", getMsgURIKey)
	reg.RegisterFunc("getSvcParams", getSvcParams)
	reg.RegisterFunc("getRestSvcOptions", getRestSvcOptions)
	reg.RegisterFunc("getMethodParams", getMethodParams)
	reg.RegisterFunc("getCWD2", getCWD2)
	reg.RegisterFunc("getSwaggerFileName", getSwaggerFileName)
	reg.RegisterFunc("createDir", createDir)
	reg.RegisterFunc("genManifest", genManifest)
	reg.RegisterFunc("genPkgManifest", genPkgManifest)
	reg.RegisterFunc("genSvcManifest", genServiceManifest)
	reg.RegisterFunc("getSvcManifest", getServiceManifest)
	reg.RegisterFunc("addRelations", addRelations)
	reg.RegisterFunc("genRelMap", genRelMap)
	reg.RegisterFunc("title", strings.Title)
	reg.RegisterFunc("detitle", detitle)
	reg.RegisterFunc("getInputType", getInputType)
	reg.RegisterFunc("getOutputType", getOutputType)
	reg.RegisterFunc("getListType", getListType)
	reg.RegisterFunc("getListTypeMsg", getListTypeMsg)
	reg.RegisterFunc("getWatchType", getWatchType)
	reg.RegisterFunc("getWatchTypeMsg", getWatchTypeMsg)
	reg.RegisterFunc("isAutoWatch", isAutoWatch)
	reg.RegisterFunc("isAutoList", isAutoList)
	reg.RegisterFunc("isWatchHelper", isWatchHelper)
	reg.RegisterFunc("isListHelper", isListHelper)
	reg.RegisterFunc("getPackageCrudObjects", getPackageCrudObjects)
	reg.RegisterFunc("getSvcCrudObjects", getSvcCrudObjects)
	reg.RegisterFunc("isAutoGenMethod", common.IsAutoGenMethod)
	reg.RegisterFunc("getAutoRestOper", getAutoRestOper)
	reg.RegisterFunc("isRestExposed", isRestExposed)
	reg.RegisterFunc("isRestMethod", isRestMethod)
	reg.RegisterFunc("isNestedMessage", isNestedMessage)
	reg.RegisterFunc("getNestedMsgName", getNestedMsgName)
	reg.RegisterFunc("isMapEntry", isMapEntry)
	reg.RegisterFunc("getFileName", getFileName)
	reg.RegisterFunc("getGrpcDestination", getGrpcDestination)
	reg.RegisterFunc("getValidatorManifest", getValidatorManifest)
	reg.RegisterFunc("derefStr", derefStr)
	reg.RegisterFunc("getEnumStrMap", getEnumStrMap)
	reg.RegisterFunc("getStorageTransformersManifest", getStorageTransformersManifest)
	reg.RegisterFunc("isSpecStatusMessage", isSpecStatusMessage)
	reg.RegisterFunc("saveBool", scratch.setBool)
	reg.RegisterFunc("getBool", scratch.getBool)
	reg.RegisterFunc("saveInt", scratch.setInt)
	reg.RegisterFunc("getInt", scratch.getInt)
	reg.RegisterFunc("saveStr", scratch.setStr)
	reg.RegisterFunc("getStr", scratch.getStr)
	reg.RegisterFunc("isAPIServerServed", isAPIServerServed)
	reg.RegisterFunc("getSvcActionEndpoints", getSvcActionEndpoints)
	reg.RegisterFunc("getDefaulterManifest", getDefaulterManifest)
	reg.RegisterFunc("getRelPath", getRelPath)
	reg.RegisterFunc("getMsgMap", getMsgMap)
	reg.RegisterFunc("getEventTypes", getEventTypes)
	reg.RegisterFunc("getFileCategory", getFileCategory)
	reg.RegisterFunc("isSvcWatch", isSvcWatch)
	reg.RegisterFunc("getAPIOperType", getAPIOperType)
	reg.RegisterFunc("getWatchHelperName", getWatchHelperName)
	reg.RegisterFunc("isStreaming", isStreaming)
	reg.RegisterFunc("isClientStreaming", isClientStreaming)
	reg.RegisterFunc("isTenanted", isTenanted)
	reg.RegisterFunc("isObjTenanted", isObjTenanted)
	reg.RegisterFunc("getProxyPaths", getProxyPaths)

	// Register request mutators
	reg.RegisterReqMutator("pensando", reqMutator)
}

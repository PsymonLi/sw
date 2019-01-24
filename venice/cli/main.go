package vcli

import (
	"bytes"
	"fmt"
	"io"
	"os"
	"regexp"
	"sort"

	"github.com/urfave/cli"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/venice/cli/gen"
)

// CLI features
// - runs as pre-packaged container
// - provide CRUD on all pensando objects: IAM, Network, Telemetry, Tenancy, Global, Internal objects
// - Allow object creation using command line, or from file/directory or list of files/directories, recursively
// - bash auto-completion for the commands options, subcommands and dynamic field values (e.g. user names)
// - read/upload formats allowed: yaml, json
// - selectable read (and delete) criteria: besides names, use labels, or regular-expressions
// - table based show for multiple objects
// - ordered read from file: allows objects to be specified in any order (declarative)
// - various global options: -d for debug, -e for examples, -s shows object definition
// - support no-op mode: to mock the cli validation, etc. no action is performed
// - quiet mode that shows only the object names, as opposed to the details
// - allow multiple object names on read command line
// - get user confirmation when deleting multiple elements (based on labels, or regular expression)
// - read only objects that disallows create or changing spec on the objects
// - label manipulation: add/remove labels to any object(s)
// - selectable output fields in summary output
// - allow updating name of an object (using uuid of the object)
// - show tree of all instantiated objects and their relationship
// - use editor based object creation, modification (to existing object)
// - incremental addition (spec changes) i.e. read-modify-write
// - configuration snapshots and restore

// TODO
// - rename "workloadLabels" to "workloadSelector"
// - template generated examples
// - add help tour
// - auth (user login, etc.)

// context structure is internal to CLI module that is passed along various functions
// to keep the context about a specific CLI command; it stores digested information
type context struct {
	cli               *cli.Context
	tenant            string
	cmd               string
	subcmd            string
	token             string
	server            string
	labels            map[string]string
	dumpStruct        bool
	dumpYml           bool
	names             []string
	quiet             bool
	debug             bool
	re                *regexp.Regexp
	structInfo        *api.Struct
	listStructInfo    *api.Struct
	genInfo           *gen.Info
	removeObjOperFunc gen.RemoveObjOperFunc
	restGetFunc       gen.RestGetFunc
	restDeleteFunc    gen.RestFunc
	restPostFunc      gen.RestFunc
	restPutFunc       gen.RestFunc
}

const (
	defaultVeniceURL = "http://localhost:9000"
	helpTmpl         = `NAME:
{{.Name}} - {{.Usage}}
USAGE:
  {{.HelpName}} {{if .VisibleFlags}}[global options]{{end}}{{if .Commands}} command [command options]{{end}} {{if .ArgsUsage}}{{.ArgsUsage}}{{else}}[arguments...]{{end}}
  {{if len .Authors}}
AUTHOR:
  {{range .Authors}}{{ . }}{{end}}
  {{end}}{{if .Commands}}
COMMANDS:
{{range .Commands}}{{if not .HideHelp}}  {{join .Names ", "}}{{ "\t"}}{{.Usage}}{{ "\n" }}{{end}}{{end}}{{end}}{{if .VisibleFlags}}
GLOBAL OPTIONS:
  {{range .VisibleFlags}}{{.}}
  {{end}}{{end}}{{if .Copyright }}
COPYRIGHT:
  {{.Copyright}}
  {{end}}{{if .Version}}
VERSION:
  {{.Version}}
{{end}}
`
)

type byName []cli.Command

func (a byName) Len() int           { return len(a) }
func (a byName) Swap(i, j int)      { a[i], a[j] = a[j], a[i] }
func (a byName) Less(i, j int) bool { return a[i].Name < a[j].Name }

// global flags for venice cli command
var penServerFlags = []cli.Flag{
	cli.BoolFlag{
		Name:  "debug, d",
		Usage: "Client Debug Enable",
	},
	cli.StringFlag{
		Name:   "server",
		Value:  defaultVeniceURL,
		Usage:  "Complete URL of the pensando server",
		EnvVar: "VENICE_URL",
	},
}

// InvokeCLI invokes pensando CLI programmatically, which is used by
// the main routine but also by unit tests, during unit tests we capture the stdout
// into a string to verify the command output/results
func InvokeCLI(osArgs []string, bot bool) string {
	var r, w, oldStdout *os.File

	if bot {
		oldStdout = os.Stdout
		r, w, _ = os.Pipe()
		os.Stdout = w
	}

	app := cli.NewApp()
	app.Flags = penServerFlags
	app.Version = DefaultVersion
	app.Copyright = "Copyright (c) Pensando Systems, Inc."
	app.Usage = "Pensando Venice CLI"

	app.EnableBashCompletion = true
	cli.BashCompletionFlag = cli.BoolFlag{
		Name:   "gbc",
		Hidden: true,
	}
	app.BashComplete = bashMainCompleter

	cli.AppHelpTemplate = helpTmpl
	sort.Sort(byName(Commands))
	if err := generateCommands(); err != nil {
		return fmt.Sprintf("error '%s' generating commands", err)
	}
	app.Commands = Commands
	app.Metadata = make(map[string]interface{})
	app.Metadata["osArgs"] = osArgs
	app.Run(osArgs)

	if bot {
		outC := make(chan string)
		go func() {
			var buf bytes.Buffer
			io.Copy(&buf, r)
			outC <- buf.String()
		}()

		w.Close()
		os.Stdout = oldStdout

		out := <-outC
		return out
	}
	return ""
}

// bash command line completer for top level commands
func bashMainCompleter(c *cli.Context) {
	bashCompleter(c, Commands, penServerFlags)
}

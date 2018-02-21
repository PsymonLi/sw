package imports

import "sync"

// export.go exports some type and func of golang.org/x/tools/imports

var (
	exportedGoPath   map[string]*Pkg
	exportedGoPathMu sync.RWMutex
)

// GoPath returns all importable packages (abs dir path => *Pkg).
func GoPath() map[string]*Pkg {
	exportedGoPathMu.Lock()
	defer exportedGoPathMu.Unlock()
	if exportedGoPath != nil {
		return exportedGoPath
	}
	populateIgnoreOnce.Do(populateIgnore)
	scanGoRootOnce.Do(scanGoRoot) // async
	scanGoPathOnce.Do(scanGoPath)
	<-scanGoRootDone
	dirScanMu.Lock()
	defer dirScanMu.Unlock()
	exportedGoPath = exportDirScan(dirScan)
	return exportedGoPath
}

func exportDirScan(ds map[string]*pkg) map[string]*Pkg {
	r := make(map[string]*Pkg)
	for path, pkg := range ds {
		r[path] = exportPkg(pkg)
	}
	return r
}

// Pkg represents exported type of pkg.
type Pkg struct {
	Dir             string // absolute file path to Pkg directory ("/usr/lib/go/src/net/http")
	ImportPath      string // full Pkg import path ("net/http", "foo/bar/vendor/a/b")
	ImportPathShort string // vendorless import path ("net/http", "a/b")
}

func exportPkg(p *pkg) *Pkg {
	return &Pkg{Dir: p.dir, ImportPath: p.importPath, ImportPathShort: p.importPathShort}
}

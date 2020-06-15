default: psmctl

psmctl:
	# TODO: add version support tools/psmctl_version.sh
	export GIT_COMMIT=`git rev-parse --short HEAD` && CGO_ENABLED=0  go build -ldflags "-X main.GitCommit=$$GIT_COMMIT -X main.BuiltTime=$$(date +%Y.%m.%d-%H:%M:%S)" -o ../venice/build/bin/psmctl.linux github.com/pensando/sw/psmctl
	GOOS=freebsd export GIT_COMMIT=`git rev-parse --short HEAD` && CGO_ENABLED=0  go build -ldflags "-X main.GitCommit=$$GIT_COMMIT -X main.BuiltTime=$$(date +%Y.%m.%d-%H:%M:%S)" -o ../venice/build/bin/psmctl.freebsd github.com/pensando/sw/psmctl
	GOOS=windows export GIT_COMMIT=`git rev-parse --short HEAD` && CGO_ENABLED=0  go build -ldflags "-X main.GitCommit=$$GIT_COMMIT -X main.BuiltTime=$$(date +%Y.%m.%d-%H:%M:%S)" -o ../venice/build/bin/psmctl.win64.exe github.com/pensando/sw/psmctl
	GOOS=windows GOARCH=386 export GIT_COMMIT=`git rev-parse --short HEAD` && CGO_ENABLED=0  go build -ldflags "-X main.GitCommit=$$GIT_COMMIT -X main.BuiltTime=$$(date +%Y.%m.%d-%H:%M:%S)" -o ../venice/build/bin/psmctl.win32.exe github.com/pensando/sw/psmctl

clean:
	rm -rf $(GOPATH)/src/github.com/pensando/sw/psmctl/bin/

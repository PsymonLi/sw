package runner

import (
	"bytes"
	"fmt"
	"io"

	"golang.org/x/crypto/ssh"

	constants "github.com/pensando/sw/iota/svcs/common"
	"github.com/pensando/sw/venice/utils/log"
)

// Runner implements all remote to command execution functions
type Runner struct {
	SSHClientConfig *ssh.ClientConfig
}

// NewRunner returns a new copier instance
func NewRunner(c *ssh.ClientConfig) *Runner {
	runner := &Runner{
		SSHClientConfig: c,
	}
	return runner
}

// Run runs a command either in foreground on on background
func (r *Runner) run(ipPort, command string, cmdMode int) (string, error) {
	client, err := ssh.Dial("tcp", ipPort, r.SSHClientConfig)
	if client == nil || err != nil {
		log.Errorf("Runner | Run on node %v failed, Err: %v", ipPort, err)
		return "", err
	}
	defer client.Close()

	session, err := client.NewSession()
	if session == nil || err != nil {
		log.Errorf("Runner | Run on node %v failed, Err: %v", ipPort, err)
		return "", err
	}
	defer session.Close()

	if cmdMode != constants.RunCommandForegroundNoShell {
		ptyModes := ssh.TerminalModes{
			ssh.TTY_OP_ISPEED: 14400,
			ssh.TTY_OP_OSPEED: 14400,
			ssh.ECHO:          0,
		}

		if err := session.RequestPty("xterm", 80, 40, ptyModes); err != nil {
			log.Errorf("Runner | Run on node %v failed to get a pseudo TTY, Err: %v", ipPort, err)
			return "", err
		}
	}

	stdoutBuffer := bytes.Buffer{}
	stdoutWriter := io.MultiWriter(&stdoutBuffer)

	//Pipe stdout and stderr
	stdout, err := session.StdoutPipe()
	if err != nil {
		log.Errorf("Runner | Run on node %v failed to get a stdout, Err: %v", ipPort, err)
		return "", fmt.Errorf("could not capture stdout. %v", err)
	}
	ioDone := make(chan bool, 1)
	ioStart := make(chan bool)

	go func() {
		ioStart <- true
		_, err = io.Copy(stdoutWriter, stdout)
		if err != nil {
			log.Errorf("Runner | Copy stdout failed %v", err)
		}
		ioDone <- true
	}()

	<-ioStart
	switch cmdMode {
	case constants.RunCommandBackground:
		log.Infof("Runner | Running command %v in background...", command)
		command = "nohup sh -c  \"" + command + " 2>&1 >/dev/null </dev/null & \""
	case constants.RunCommandForeground:
		command = "sh -c \"" + command + "\""
	}

	if err := session.Run(command); err != nil {
		log.Errorf("Runner | Command %v failed on node %v Err: %v", command, ipPort, err)
		return "", err
	}

	<-ioDone
	return stdoutBuffer.String(), nil
}

// RunWithOutput runs a command either in foreground on on background with stdout
func (r *Runner) RunWithOutput(ipPort, command string, cmdMode int) (string, error) {
	return r.run(ipPort, command, cmdMode)
}

// Run runs a command either in foreground on on background
func (r *Runner) Run(ipPort, command string, cmdMode int) error {
	_, err := r.run(ipPort, command, cmdMode)
	return err
}

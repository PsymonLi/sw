package main

import (
	"flag"

	"github.com/pensando/sw/iota/svcs/agent"
)

func main() {
	var restore string
	var stubMode = flag.Bool("stubmode", false, "Start IOTA Agent in stub mode")
	flag.StringVar(&restore, "restore", "", "restore state")
	flag.Parse()
	agent.StartIOTAAgent(stubMode, &restore)
}

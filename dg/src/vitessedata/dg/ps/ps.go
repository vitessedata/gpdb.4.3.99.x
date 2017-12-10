package ps

import (
	"flag"
	"fmt"
	"github.com/shirou/gopsutil/process"
	"os"
)

func extract(pid int32) {
	p, err := process.NewProcess(pid)
	if err != nil {
		return
	}

	// pid, ppid, uid, euid, gid, egid, command
	uids, err := p.Uids()
	if err != nil {
		return
	}

	gids, err := p.Gids()
	if err != nil {
		return
	}

	ppid, err := p.Ppid()
	if err != nil {
		return
	}

	line, err := p.Cmdline()
	if err != nil {
		return
	}

	fmt.Printf("%d %d %d %d %d %d %s\n",
		pid, ppid,
		uids[0], // real
		uids[1], // effective
		gids[0], // real
		gids[1], // effective
		line)
}

func Command(args []string) {
	// ps [-p pid]
	flagset := flag.NewFlagSet("ps", flag.ExitOnError)
	readPid := flagset.Int("p", 0, "specific pid.")

	if err := flagset.Parse(args); err != nil {
		fmt.Fprintf(os.Stderr, "Usage: %s ps\n", os.Args[0])
		os.Exit(1)
	}

	args = flagset.Args()
	if len(args) != 0 {
		fmt.Fprintf(os.Stderr, "Usage: %s ps\n", os.Args[0])
		os.Exit(1)
	}

	if *readPid != 0 {
		extract(int32(*readPid))
		return
	}

	pids, err := process.Pids()
	if err != nil {
		fmt.Fprintf(os.Stderr, "ps: %s\n", err)
		os.Exit(1)
	}

	for _, pid := range pids {
		extract(pid)
	}
}

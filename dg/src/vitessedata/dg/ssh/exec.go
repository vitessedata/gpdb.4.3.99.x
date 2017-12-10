package ssh

import (
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"vitessedata/dg/dglog"
)

type ExecResult struct {
	Host string
	Err  error
	Out  string
}

func findDg() string {
	if filepath.Base(os.Args[0]) == "dg" && filepath.IsAbs(os.Args[0]) {
		// dglog.Log("Using dg from %s\n", os.Args[0])
		return os.Args[0]
	}

	// dg should be dghome/bin/dg, base is dghome/bin, so base/.. is what we need.
	dgpath, err := exec.Command("sh", "-c", "which dg").Output()
	dglog.FatalErr(err, "Cannot find path of dg")
	// dglog.Log("Using dg from %s\n", string(dgpath))
	return strings.TrimSpace(string(dgpath))
}

var g_dgbin = findDg()

func ExecAnyError(res []ExecResult) error {
	for _, r := range res {
		if r.Err != nil {
			return r.Err
		}
	}
	return nil
}

func execOnOneHost(user string, host string, cmd string, c chan ExecResult) {
	result := ExecResult{host, nil, ""}
	var args []string
	var bin string

	if user == "" {
		bin = g_dgbin
		args = []string{"ssh", host, cmd}
	} else {
		bin = "sudo"
		args = []string{"-u", user, g_dgbin, "ssh", host, cmd}
	}

	dglog.Log("Will run command on host %s, cmd: %v", host, args)
	xcmd := exec.Command(bin, args...)
	out, err := xcmd.Output()
	result.Err = err
	if result.Err == nil {
		result.Out = string(out)
	} else {
		errtxt := err.(*exec.ExitError).Stderr
		dglog.Log("Error: stderr %s, err %v", errtxt, err)
	}
	c <- result
}

func ExecCmdOnEachHost(r string, hosts []string, cmd string, result chan ExecResult) {
	for _, h := range hosts {
		go execOnOneHost(r, h, cmd, result)
	}
}

func ExecCmdArrOnEachHost(r string, hosts []string, cmds []string, result chan ExecResult) {
	for i := 0; i < len(hosts); i++ {
		go execOnOneHost(r, hosts[i], cmds[i], result)
	}
}

func ExecCmdOn(r string, hosts []string, cmd string) []ExecResult {
	ch := make(chan ExecResult)
	res := make([]ExecResult, len(hosts))
	ExecCmdOnEachHost(r, hosts, cmd, ch)

	for nchecked := 0; nchecked < len(hosts); nchecked++ {
		res[nchecked] = <-ch
	}
	return res
}

func ExecOn(r string, hosts []string, cmds []string) []ExecResult {
	ch := make(chan ExecResult)
	res := make([]ExecResult, len(hosts))
	ExecCmdArrOnEachHost(r, hosts, cmds, ch)

	for nchecked := 0; nchecked < len(hosts); nchecked++ {
		res[nchecked] = <-ch
	}
	return res
}

func MyHostName() (string, error) {
	h := []string{"localhost"}
	res := ExecCmdOn("", h, "hostname")
	if res[0].Err != nil {
		return "", res[0].Err
	}
	return strings.TrimSpace(res[0].Out), nil
}

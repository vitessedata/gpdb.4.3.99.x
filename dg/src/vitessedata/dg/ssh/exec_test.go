package ssh

import (
	"fmt"
	"testing"
)

func runCmd(r string, hosts []string, cmd string) bool {
	ok := true
	ch := make(chan ExecResult)
	ExecCmdOnEachHost(r, hosts, cmd, ch)
	for i, _ := range hosts {
		r := <-ch
		if r.Err != nil {
			ok = false
			fmt.Printf("Host # %d, %s %s\n", i, r.Host, r.Err.Error())
		} else {
			fmt.Printf("Host # %d, %v\n", i, r)
		}
	}
	return ok
}

func TestExec(t *testing.T) {
	mlcmd := `printf '
this is a multi line cmd
line2
line3
' > /tmp/xxx.txt`
	t.Run("exec", func(t *testing.T) {
		hosts := []string{"dg0", "dg1", "dg2"}
		ok := runCmd("deepgreen", hosts, "hostname; hostname")
		if !ok {
			t.Error("Running hostname failed")
		}

		ok = runCmd("", hosts, "hostname; hostname")
		if !ok {
			t.Error("Running hostname as root failed")
		}

		ok = runCmd("deepgreen", hosts, mlcmd)
		if !ok {
			t.Error("printf failed.")
		}

		ok = runCmd("root", hosts, "echo foo; echo bar; echo zoo")
		if !ok {
			t.Error("echo 3 lines failed.")
		}
	})
}

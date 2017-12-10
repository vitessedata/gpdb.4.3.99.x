package ssh

import (
	"flag"
	"fmt"
	"golang.org/x/crypto/ssh"
	"golang.org/x/crypto/ssh/agent"
	"io/ioutil"
	"net"
	"os"
	"os/user"
)

func sshAgent() ssh.AuthMethod {
	if ag, err := net.Dial("unix", os.Getenv("SSH_AUTH_SOCK")); err == nil {
		return ssh.PublicKeysCallback(agent.NewClient(ag).Signers)
	}
	return nil
}

func getKeyFile(usr *user.User) (key ssh.Signer, err error) {
	file := usr.HomeDir + "/.ssh/id_rsa"
	buf, err := ioutil.ReadFile(file)
	if err != nil {
		return
	}
	key, err = ssh.ParsePrivateKey(buf)
	if err != nil {
		return
	}
	return
}

func Command(args []string) {
	// dg ssh [-p] [-user=] [-passwd=] host commands
	flagset := flag.NewFlagSet("ssh", flag.ExitOnError)
	readPassword := flagset.Bool("P", false, "Obtain password from environ var SSHPASSWORD.")

	argUser := flagset.String("user", "", "ssh user")

	if err := flagset.Parse(args); err != nil {
		fmt.Fprintf(os.Stderr, "Usage: %s ssh [-P] host command\n", os.Args[0])
		os.Exit(1)
	}

	args = flagset.Args()
	if len(args) != 2 {
		fmt.Fprintf(os.Stderr, "Usage: %s ssh [-P] host command\n", os.Args[0])
		os.Exit(1)
	}

	host := args[0] + ":22"
	cmd := args[1]

	/* Get the current user */
	usr, err := user.Current()
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}

	/* Set up the ssh config */
	var config *ssh.ClientConfig
	if *argUser == "" {
		config = &ssh.ClientConfig{
			User:            usr.Username,
			HostKeyCallback: ssh.InsecureIgnoreHostKey(),
		}
	} else {
		config = &ssh.ClientConfig{
			User:            *argUser,
			HostKeyCallback: ssh.InsecureIgnoreHostKey(),
		}
	}
	config.Auth = make([]ssh.AuthMethod, 0, 3)

	/* if user wants to provide password ... */
	if *readPassword {
		password := os.Getenv("SSHPASSWORD")
		if password == "" {
			fmt.Fprintln(os.Stderr,
				"ERROR: please provide password in env SSHPASSWORD")
			os.Exit(1)
		}
		config.Auth = append(config.Auth, ssh.Password(password))
	} else {
		/* next option: is there an ssh agent */
		if ag := sshAgent(); ag != nil {
			config.Auth = append(config.Auth, ag)
		}

		/* finally, try to use my public key */
		key, err := getKeyFile(usr)
		if err == nil {
			config.Auth = append(config.Auth, ssh.PublicKeys(key))
		}

		/* if no auth method found, then error out */
		if len(config.Auth) == 0 {
			if err != nil {
				fmt.Fprintln(os.Stderr, err)
			} else {
				fmt.Fprintln(os.Stderr, "ERROR: no authentication method found")
			}
			os.Exit(1)
		}
	}

	client, err := ssh.Dial("tcp", host, config)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}

	session, err := client.NewSession()
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
	defer session.Close()

	session.Stdin = os.Stdin
	session.Stdout = os.Stdout
	session.Stderr = os.Stderr

	err = session.Run(cmd)
	if err != nil {
		if nerr, ok := err.(*ssh.ExitError); ok {
			os.Exit(nerr.ExitStatus())
		}
		if nerr, ok := err.(*ssh.ExitMissingError); ok {
			fmt.Fprintln(os.Stderr, nerr)
			os.Exit(1)
		}
		fmt.Fprintln(os.Stderr, "Command failed.")
		os.Exit(1)
	}
	os.Stdin.Close()
}

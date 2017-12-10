package main

import (
	"fmt"
	"os"
	"vitessedata/dg/db"
	"vitessedata/dg/ps"
	"vitessedata/dg/ssh"
)

var usage string = `
Usage: dg <command> [<flags>] [<args>]

Commands for deepgreen internal use,
	ssh		
	db
	ps

Help
	help		Print this message.
`

func main() {

	if len(os.Args) == 1 {
		fmt.Fprintf(os.Stderr, usage)
		os.Exit(1)
	}

	switch os.Args[1] {

	case "ssh":
		ssh.Command(os.Args[2:])

	case "ps":
		ps.Command(os.Args[2:])

	case "db":
		db.Command(os.Args[2:])

	case "help":
		fmt.Fprintf(os.Stderr, usage)

	default:
		fmt.Fprintf(os.Stderr, "Error: %s is not a valid command\n", os.Args[1])
		os.Exit(1)
	}
}

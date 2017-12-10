package db

import (
	"flag"
	"fmt"
	"os"
	"os/exec"
	"vitessedata/dg/dglog"
)

func GpStart(md string) error {
	dglog.Log("Startring deepgreen database ...")
	_, err := exec.Command("gpstart", "-a", "-d", md).Output()
	dglog.LogErr(err, "gpstart failed.")
	return err
}

func GpStop(md string) error {
	_, err := exec.Command("gpstop", "-a", "-M", "fast", "-d", md).Output()
	dglog.LogErr(err, "gpstop failed.")
	return err
}

func GpStartMasterOnly(md string) error {
	cmd := fmt.Sprintf("yes | gpstart -a -m -d %s", md)
	dglog.Log("Starting deepgreen in utility mode, cmd: %s", cmd)
	_, err := exec.Command("bash", "-c", cmd).Output()
	dglog.LogErr(err, "gpstart failed.")
	return err
}

func GpStopMasterOnly(md string) error {
	dglog.Log("Stopping deepgreen in utility mode, md is %s", md)
	_, err := exec.Command("gpstop", "-a", "-m", "-M", "fast", "-d", md).Output()
	dglog.LogErr(err, "gpstop failed.")
	return err
}

func RunSQL(db string, port string, sql string) (string, error) {
	dbname := fmt.Sprintf("--dbname=%s", db)
	portarg := fmt.Sprintf("--port=%s", port)
	args := []string{dbname, portarg, "-t", "-c", sql}
	out, err := exec.Command("psql", args...).Output()
	dglog.LogErr(err, "psql failed.")
	return string(out), err
}

func RunSQLFile(db string, port string, fn string) (string, error) {
	dbname := fmt.Sprintf("--dbname=%s", db)
	portarg := fmt.Sprintf("--port=%s", port)
	args := []string{dbname, portarg, "-t", "-f", fn}
	out, err := exec.Command("psql", args...).Output()
	dglog.LogErr(err, "psql failed.")
	return string(out), err
}

func RunSQLUtilityMode(db string, port string, sql string) (string, error) {
	dbname := fmt.Sprintf("--dbname=%s", db)
	portarg := fmt.Sprintf("--port=%s", port)
	cmd := fmt.Sprintf("export PGOPTIONS='-c gp_session_role=utility'; psql %s %s -t -c \"%s\"", dbname, portarg, sql)

	dglog.Log("Run utility mode, cmd %s\n", cmd)
	out, err := exec.Command("sh", "-c", cmd).Output()
	dglog.LogErr(err, "psql (utility mode) failed.")
	return string(out), err
}

func RunSQLFileUtilityMode(db string, port string, fn string) (string, error) {
	dbname := fmt.Sprintf("--dbname=%s", db)
	portarg := fmt.Sprintf("--port=%s", port)
	cmd := fmt.Sprintf("export PGOPTIONS='-c gp_session_role=utility'; psql %s %s -t -f %s", dbname, portarg, fn)

	dglog.Log("Run utility mode, cmd %s\n", cmd)
	out, err := exec.Command("sh", "-c", cmd).Output()
	dglog.LogErr(err, "psql (utility mode) failed.")
	return string(out), err
}

func Command(args []string) {
	flagset := flag.NewFlagSet("db", flag.ExitOnError)
	md := flagset.String("md", "", "master data dir")
	db := flagset.String("db", "", "database name")
	port := flagset.String("port", "5432", "database port")
	cmd := flagset.String("cmd", "", "command")
	sql := flagset.String("sql", "", "sql")

	if err := flagset.Parse(args); err != nil {
		fmt.Fprintf(os.Stderr, "Usage: %s db\n", os.Args[0])
		flagset.PrintDefaults()
		os.Exit(1)
	}

	args = flagset.Args()
	if len(args) != 0 {
		fmt.Fprintf(os.Stderr, "Usage: %s db\n", os.Args[0])
		flagset.PrintDefaults()
		os.Exit(1)
	}

	switch *cmd {
	case "start":
		GpStart(*md)
	case "stop":
		GpStop(*md)
	case "startu":
		GpStartMasterOnly(*md)
	case "stopu":
		GpStopMasterOnly(*md)
	case "sql":
		RunSQL(*db, *port, *sql)
	case "sqlf":
		RunSQLFile(*db, *port, *sql)
	case "sqlu":
		RunSQLUtilityMode(*db, *port, *sql)
	case "sqlfu":
		RunSQLFileUtilityMode(*db, *port, *sql)
	default:
		fmt.Fprintf(os.Stderr, "Invalid comnand %s", *cmd)
		os.Exit(1)
	}
}

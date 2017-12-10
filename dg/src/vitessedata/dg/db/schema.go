package db

import (
	"fmt"
)

func (db *DeepgreenDB) HasSchema(s string) (bool, error) {
	rows, err := db.DBConn.Query("SELECT nspname FROM pg_catalog.pg_namespace WHERE nspname = $1", s)
	if err != nil {
		return false, err
	}
	defer rows.Close()

	for rows.Next() {
		return true, nil
	}
	return false, nil
}

func (db *DeepgreenDB) DropSchema(s string) error {
	// SQL Injection my ass.
	sql := fmt.Sprintf("DROP schema if exists %s cascade", s)
	return db.Execute(sql)
}

func (db *DeepgreenDB) ListUserSchema() ([]string, error) {
	ret := make([]string, 0)
	sql := `select n.nspname from pg_catalog.pg_namespace n 
	where n.nspname not in ('gpexpand', 'information_schema',
		'gp_toolkit', 'dg_utils') and n.nspname not like 'pg_%' `
	rows, err := db.DBConn.Query(sql)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	for rows.Next() {
		var s string
		rows.Scan(&s)
		ret = append(ret, s)
	}
	return ret, nil
}

func (db *DeepgreenDB) ListDatabase() ([]string, error) {
	ret := make([]string, 0)
	sql := "select datname from pg_database"
	rows, err := db.DBConn.Query(sql)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	for rows.Next() {
		var s string
		rows.Scan(&s)
		ret = append(ret, s)
	}
	return ret, nil
}

func (db *DeepgreenDB) HasDatabase(dbName string) (bool, error) {
	rows, err := db.DBConn.Query("SELECT datname FROM pg_database where datname = $1", dbName)
	if err != nil {
		return false, err
	}
	defer rows.Close()

	for rows.Next() {
		return true, nil
	}
	return false, nil
}

func (db *DeepgreenDB) CreateDatabase(dbname string) error {
	return db.Execute("Create database " + dbname)
}

package db

import (
	"fmt"
)

type UserTable struct {
	Schema string
	Table  string
}

func (db *DeepgreenDB) listTopLvUserTables(schema string, nox bool) ([]UserTable, error) {
	tables := make([]UserTable, 0)
	sql := `SELECT n.nspname, c.relname
			FROM pg_class c JOIN pg_namespace n on (c.relnamespace=n.oid)
				JOIN pg_catalog.gp_distribution_policy p on (c.oid = p.localoid)
			WHERE c.oid NOT IN ( SELECT parchildrelid as oid FROM pg_partition_rule) 
			`

	if schema != "" {
		sql = fmt.Sprintf("%s AND n.nspname = '%s'", sql, schema)
	} else {
		sql = sql + " AND n.nspname NOT IN ('gpexpand', 'pg_bitmapindex', 'information_schema', 'gp_toolkit', 'dg_utils') "
	}

	if nox {
		sql = sql + " AND c.relstorage <> 'x'"
	}

	rows, err := db.DBConn.Query(sql)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	for rows.Next() {
		var t UserTable
		rows.Scan(&t.Schema, &t.Table)
		tables = append(tables, t)
	}
	return tables, nil
}

func (db *DeepgreenDB) TopLvUserTables(schema string) ([]UserTable, error) {
	return db.listTopLvUserTables(schema, false)
}
func (db *DeepgreenDB) TopLvUserTablesNoX(schema string) ([]UserTable, error) {
	return db.listTopLvUserTables(schema, true)
}

func (db *DeepgreenDB) IsPartitioned(schema string, tab string) (bool, error) {
	rows, err := db.DBConn.Query("select * from pg_partitions where schemaname = $1 and tablename = $2", schema, tab)
	if err != nil {
		return false, err
	}
	defer rows.Close()
	for rows.Next() {
		return true, nil
	}
	return false, nil
}

func (db *DeepgreenDB) TableExists(ut UserTable) (bool, error) {
	sql := `SELECT n.nspname, c.relname
			FROM pg_class c JOIN pg_namespace n on (c.relnamespace=n.oid)
			WHERE n.nspname = $1 and c.relname = $2
			`
	rows, err := db.DBConn.Query(sql, ut.Schema, ut.Table)
	if err != nil {
		return false, err
	}
	defer rows.Close()
	for rows.Next() {
		return true, nil
	}
	return false, nil
}

type Column struct {
	Name     string
	TypeId   int32
	TypeName string
}

func (db *DeepgreenDB) TableColumns(ut UserTable) ([]Column, error) {
	sql := `SELECT att.attname, t.oid, t.typname
			FROM pg_class c, pg_namespace n, pg_attribute att, pg_type t
			WHERE n.nspname = $1 and c.relname = $2
			and c.relnamespace = n.oid and c.oid = att.attrelid 
			and att.attnum > 0 and att.atttypid = t.oid 
			ORDER BY att.attnum
			`

	ret := make([]Column, 0)
	rows, err := db.DBConn.Query(sql, ut.Schema, ut.Table)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	for rows.Next() {
		var c Column
		rows.Scan(&c.Name, &c.TypeId, &c.TypeName)
		ret = append(ret, c)
	}
	return ret, nil
}

package db

import (
	"database/sql"
	"fmt"
	_ "github.com/lib/pq"
)

type DeepgreenDB struct {
	DBHost string
	DBPort string
	DBUser string
	DB     string
	DBConn *sql.DB
}

func (db *DeepgreenDB) connect() error {
	if db.DBConn != nil {
		return fmt.Errorf("Arleady connected")
	}

	var err error
	var connstr string
	if db.DBUser == "" {
		connstr = fmt.Sprintf("postgres://%s:%s/%s?sslmode=disable", db.DBHost, db.DBPort, db.DB)
	} else {
		connstr = fmt.Sprintf("postgres://%s@%s:%s/%s?sslmode=disable", db.DBUser, db.DBHost, db.DBPort, db.DB)
	}

	db.DBConn, err = sql.Open("postgres", connstr)
	return err
}

func Connect(host string, port string, user string, db string) (*DeepgreenDB, error) {
	// DeepGreen is the only flavor.
	var env DeepgreenDB
	env.DBHost = host
	env.DBPort = port
	env.DBUser = user
	env.DB = db

	err := env.connect()
	if err != nil {
		return nil, err
	}
	return &env, nil
}
func (db *DeepgreenDB) Disconnect() {
	if db.DBConn != nil {
		db.DBConn.Close()
		db.DBConn = nil
	}
}

func (db *DeepgreenDB) Execute(sql string) error {
	_, err := db.DBConn.Exec(sql)
	return err
}

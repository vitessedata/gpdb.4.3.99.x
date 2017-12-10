package db

import (
	"fmt"
	"testing"
)

func TestSchema(t *testing.T) {
	db, err := Connect("localhost", "5432", "", "dgtest")
	if err != nil {
		t.Error(err.Error())
	}
	defer db.Disconnect()

	t.Run("schema", func(t *testing.T) {
		_, err := db.DBConn.Exec("Create Schema testschema")
		if err != nil {
			t.Error(err.Error())
		}

		ok, err := db.HasSchema("testschema")
		if !ok || err != nil {
			t.Error("Does not have testschema")
		}

		ok, err = db.HasSchema("shouldNotBeThere")
		if ok || err != nil {
			t.Error("Has shouldNotBeThere schema")
		}

		err = db.DropSchema("testschema")
		if err != nil {
			t.Error("Drop schema failed. " + err.Error())
		}

		ok, err = db.HasSchema("testschema")
		if ok || err != nil {
			t.Error("Has ghost schema.")
		}

		ss, err := db.ListUserSchema()
		if err != nil {
			t.Error("List schema failed")
		}
		for _, s := range ss {
			fmt.Printf("Schema %s\n", s)
		}
	})
}

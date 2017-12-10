package db

import (
	"fmt"
	"testing"
)

func TestUserTable(t *testing.T) {
	db, err := Connect("localhost", "5432", "", "dgtest")
	if err != nil {
		t.Error(err.Error())
	}
	defer db.Disconnect()

	t.Run("list", func(t *testing.T) {
		fmt.Print("======= User tables =====================================\n")
		uts, err := db.TopLvUserTables("")
		if err != nil {
			t.Error(err.Error())
		}
		for _, ut := range uts {
			fmt.Printf("%s %s\n", ut.Schema, ut.Table)
		}

		fmt.Print("======= User tables NOX =================================\n")
		uts, err = db.TopLvUserTablesNoX("")
		if err != nil {
			t.Error(err.Error())
		}
		for _, ut := range uts {
			fmt.Printf("%s %s\n", ut.Schema, ut.Table)
		}

		fmt.Print("======= User tables NOX in s1 =============================\n")
		uts, err = db.TopLvUserTablesNoX("s1")
		if err != nil {
			t.Error(err.Error())
		}
		for _, ut := range uts {
			fmt.Printf("%s %s\n", ut.Schema, ut.Table)
		}

		fmt.Print("======= User tables . should not exists. =================\n")
		var emptyUt UserTable
		exists, err := db.TableExists(emptyUt)
		if err != nil {
			t.Error(err.Error())
		}
		if exists {
			t.Error("Bug!,  . should not exists!")
		}
	})
}

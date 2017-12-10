package db

type Segment struct {
	SegId    int
	Role     string
	Status   string
	Port     string
	HostName string
	Addr     string
}

func (db *DeepgreenDB) ListSegments() ([]Segment, error) {
	segs := make([]Segment, 0)
	sql := `SELECT content, role, status, port, hostname, address 
			FROM gp_segment_configuration
			`
	rows, err := db.DBConn.Query(sql)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	for rows.Next() {
		var seg Segment
		rows.Scan(&seg.SegId, &seg.Role, &seg.Status, &seg.Port, &seg.HostName, &seg.Addr)
		segs = append(segs, seg)
	}
	return segs, nil
}

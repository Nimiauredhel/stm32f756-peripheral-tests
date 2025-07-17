sqlite3 tests.db <<EOF
.mode table ;
SELECT * FROM requests ;
SELECT * FROM results ;
EOF

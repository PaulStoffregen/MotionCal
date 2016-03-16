#! /bin/sh

# copy a file to Windows machine for testing

ftp xp << EOT
pass
bin
put $1
EOT


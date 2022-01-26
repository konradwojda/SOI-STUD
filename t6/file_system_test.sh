#!/bin/bash

##########################################

# Testing schemas:
# 1 - creating new disc and checking it's size
# 2 - making single and nested directories
# 3 - copying file to virtual disc
# 4 - copying file from virtual disc - check diff
# 5 - link file
# 6 - unlink file
# 7 - increment file size
# 8 - decrement file size
# 9 - remove given file and directory

##########################################

set -e

chosen_schema=$1

vd_name="vd_test"

schema_1()
{
    echo "Creating new virtual disc"
    ./fs create $vd_name 1048576
    echo
    echo "Informations about root directory:"
    echo
    ./fs $vd_name ls /
}

schema_2()
{
    echo "Making directory \"a\" in root"
    ./fs $vd_name mkdir a
    echo "Making nested directories b/c in root"
    ./fs $vd_name mkdir b/c
    echo
    echo "Root directory informations:"
    ./fs $vd_name ls /
    echo
    echo "b directory informations:"
    ./fs $vd_name ls b
}

schema_3()
{
    ./fs $vd_name ls /
    echo "Copying small file to \"a\" directory and large file to root."
    ./fs $vd_name cpto short a
    echo
    echo "\"a\" directory informations:"
    ./fs $vd_name ls a
    ./fs $vd_name cpto long /
    echo
    echo "Root directory informations:"
    ./fs $vd_name ls /
}

schema_4()
{
    echo "Copying small and large file from disc and checking difference"
    echo "Informations about disc before copying: "
    ./fs $vd_name ls /
    ./fs $vd_name cpfrom a short short_copy
    ./fs $vd_name cpfrom / long long_copy
    echo
    echo "Informations after copying: "
    ./fs $vd_name ls /
    diff short short_copy
    diff long long_copy
    rm short_copy
    rm long_copy
}

schema_5()
{
    echo "Making link to long file in b/c directory"
    ./fs $vd_name link / long b/c long_link
    echo "Informations about b/c directory:"
    ./fs $vd_name ls b/c
}

schema_6()
{
    echo "Removing link to long file in b/c directory"
    ./fs $vd_name unlink b/c long_link
    echo "Informations about b/c directory:"
    ./fs $vd_name ls b/c
}

schema_7()
{
    echo "Incrementing short file size by 1000 bytes"
    echo "Information before incrementing: "
    ./fs $vd_name ls a
    ./fs $vd_name inc a short 1000
    echo
    echo "Information after incrementing: "
    ./fs $vd_name ls a
}

schema_8()
{
    echo "Decrementing short file size by 1000 bytes"
    echo "Information before decrementing: "
    ./fs $vd_name ls a
    ./fs $vd_name dec a short 1000
    echo
    echo "Information after decrementing: "
    ./fs $vd_name ls a
}

schema_9()
{
    echo "Removing big file from root"
    echo
    ./fs $vd_name ls /
    ./fs $vd_name rm / long
    echo "Informations after remove:"
    ./fs $vd_name ls /
    echo
    echo "Removing \"a\" directory (with short file)"
    ./fs $vd_name rm a short
    echo "Informations after remove:"
    ./fs $vd_name ls a

}

case $chosen_schema in
    "1") schema_1 ;;
    "2") schema_2 ;;
    "3") schema_3 ;;
    "4") schema_4 ;;
    "5") schema_5 ;;
    "6") schema_6 ;;
    "7") schema_7 ;;
    "8") schema_8 ;;
    "9") schema_9 ;;
    *) echo "Unknown command" ;;
esac
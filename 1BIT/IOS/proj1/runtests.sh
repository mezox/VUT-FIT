#!/bin/sh
# Autor: Tomas Kubovcik, xkubov02@stud.fit.vutbr.cz
# Datum: 25/03/2012
# Predmet: IOS, proj1

export LC_ALL=C

# napoveda k programu
help="Usage: $0 [-vtrsc] TEST_DIR [REGEX] 

    -v  validate tree
    -t  run tests
    -r  report results
    -s  synchronize expected results
    -c  clear generated files

    It is mandatory to supply at least one option.
"

# logicke premenne pre jednotlive prepinace
v_prep=false
t_prep=false
r_prep=false
s_prep=false
c_prep=false
nv_prep=false	#non valid option
res_failed=false
chyba_v=false

# **** Getopts ****
# spracovanie parametrov
# je povinne zadat minimalne jeden prepinac
# ak je pocet parametrov 0 dojde k ukonceniu skriptu s hodnotou 2
# + vypis napovedy k pouzitiu skriptu

if [ "$#" -lt 1 ]; then
	echo "$help" >&2
	exit 2
else
	while getopts vtrsc opt
	do
		case "$opt" in
		v)
			if [ "$v_prep" = false ]; then
				v_prep=true
			fi	;;
		t)
			if [ "$t_prep" = false ]; then
				t_prep=true
			fi	;;
		r)
			if [ "$r_prep" = false ]; then
				r_prep=true
			fi	;;
		s)
			if [ "$s_prep" = false ]; then
				s_prep=true
			fi	;;
		c)
			if [ "$c_prep" = false ]; then
				c_prep=true
			fi	;;
		*)
			if [ "$nv_prep" = false ]; then
				nv_prep=true
				echo "$help" >&2
				exit 2
			fi	;;
	esac
	done
fi

if [ v_prep=true ] || [ t_prep=true ] || [ r_prep=true ] || [ s_prep=true ] || [ c_prep=true ]; then 
		:
	else
		echo "$help" >&2
		exit 2
	fi

# shift nacitanych retazcov
shift $(($OPTIND - 1))

#do premennej TEST_DIR sa ulozi korenovy adresar sub. stromu
TEST_DIR="$1"
REGEX="$2"

# testuje sa, ci bol zadany parameter obsahujuci strom,
# ci dany subor existuje a ci je adresar 
if [ -z "$TEST_DIR" ]; then
	echo "Missing dir.tree parameter!" >&2
elif [ ! -e "$TEST_DIR" ]; then
	echo "Directory does not exist!" >&2
elif [ ! -d "$TEST_DIR" ]; then
	echo "Input parameter is not directory!" >&2
fi
	cd "$TEST_DIR"

#funkcia pre kontrolu mixu suborov a adresarov
fileintree(){ 
    dir=0
    file=0 
     
    for tree in `ls | grep -E "$REGEX"`
    do 
		if [ -f "$tree" ]; then
			file=$(($file+1))
		else
			dir=$(($dir+1))

    		if [ $file -ge 1 ]; then
       			if [ $dir -ge 1 ]; then
          			echo "Mixed directories and other files in "$tree" !"
          			chyba_v=true
       			fi
			fi
			(cd $tree; fileintree)
		fi

	    if [ -L "$tree" ]; then 
             echo ""$tree" is symbolic link!"
             chyba_v=1
    	fi
    done
}

# ********* Prepinac -v**********

if [ "$v_prep" = true ]; then
chyba_v=false

	# test obsahu adresara"
	fileintree

	# overenie spustitelnosti cmd-given"
	set $(find . -name cmd-given | grep -E "$REGEX" | sort)

	while [ "$1" != "" ]; do
		if [ ! -x "$1" ] ; then
			echo "File "$1" is not executable!"
			chyba_v=true
		fi
		shift
	done

	# overenie pristupu na citanie suborov stdin-given"
	set $(find . -name stdin-given | grep -E "$REGEX" | sort)

	while [ "$1" != "" ]; do
		if [ ! -r "$1" ] ; then
			echo "File "$1" is not readable!"
			chyba_v=true
		fi
		shift
	done

	# test suborov k zapisu"
	set $(find . -name "*-captured" -o -name "*-expected" -o -name "*-delta" | grep -E "$REGEX" | sort)

	while [ "$1" != "" ]; do
		if [ ! -w "$1" ]; then
			echo "File "$1" is not writeable!"
			chyba_v=true
		fi
		shift
	done

	# overenie suborov status* ci obsahuju cele cislo a 0x0A"
	set $(find . -name "status-captured" -o -name "status-expected" | grep -E "$REGEX" | sort)

	while [ "$1" != "" ]; do
		if [ ! "`cat "$1" | grep -E "^[0-9]+$"`" ]; then
			echo "not a number in "$1" "
			chyba_v=true
		fi
		shift
	done

	# hlada nepovolene subory
	set $(find . ! -type d | grep -E "$REGEX" | grep -vE "((stdout|stderr|status)-(expected|captured|delta))|((cmd|stdin)-(given))")

	while [ "$1" != "" ]; do
		echo "Unexpected file in: "$TEST_DIR""
		chyba_v=true
		shift
	done
fi

# ******** Prepinac -t ********

if [ "$t_prep" = true ]; then
		set $(find . -name cmd-given | grep -E "$REGEX" | sort)
		res_ok=0

while [ "$1" != "" ]; do
	WD=$(dirname "$1")
	TEST="${WD#./}"

	#ulozi povodny aktualny pwd pre navrat
	save_pwd=`pwd`

	#presun do adresara so subormi cmd-given
	cd "$TEST"
	
	if [ -e stdin-given ] ; then
		./cmd-given <stdin-given 2>stderr-captured >stdout-captured
		echo "$?" >status-captured
	else
		./cmd-given </dev/null 2>stderr-captured >stdout-captured
		echo "$?" >status-captured
	fi

	for filetype in stdin stdout stderr; do
		if [ -e "$filetype"-expected ] && [ -e "$filetype"-captured ]; then
			diff -up "$filetype"-expected "$filetype"-captured >"$filetype"-delta
		fi
	
		if [ ! -s "$filetype"-delta ]; then
			res_ok=$(($res_ok + 1))
		fi
	done

	if [ $res_ok = 3 ]; then
		RESULT="OK"
		if [ -t 2 ]; then
			echo "$TEST": "\033[1;32m"$RESULT"\033[0m" >&2
		else
			echo "$TEST": "$RESULT" >&2
		fi
	else
		RESULT="FAILED"
		res_failed=true
		if [ -t 2 ]; then
			echo "$TEST": "\033[1;31m"$RESULT"\033[0m" >&2
		else
			echo "$TEST": "$RESULT" >&2
		fi
	fi

	while [ "$save_pwd" != "$PWD" ]; do
		cd "../"
	done

	res_ok=0
	shift
done
fi

# ********** Prepinac -r **************

if [ "$r_prep" = true ]; then
	set $(find . -name cmd-given | sort | grep -E "$REGEX")
	res_ok=0;

	while [ "$1" != "" ]; do
		WD=$(dirname "$1")
		TEST="${WD#./}"
		save_pwd=`pwd`

	cd "$TEST"

	for filetype in stdin stdout stderr; do
		if [ -e "$filetype"-expected ] && [ -e "$filetype"-captured ]; then
			diff -up "$filetype"-expected "$filetype"-captured >"$filetype"-delta
		fi
	
		if [ ! -s "$filetype"-delta ]; then
			res_ok=$(($res_ok + 1))
		fi
	done
	
	if [ $res_ok = 3 ]; then
		RESULT="OK"
		if [ -t 1 ]; then
			echo "$TEST": "\033[1;32m"$RESULT"\033[0m"
		else
			echo "$TEST": "$RESULT"
		fi
	else
		RESULT="FAILED"
		res_failed=true
		if [ -t 1 ]; then
			echo "$TEST": "\033[1;31m"$RESULT"\033[0m"
		else
			echo "$TEST": "$RESULT"
		fi
	fi

	while [ "$save_pwd" != "$PWD" ]; do
		cd "../"
	done

	res_ok=0
	shift
done
fi

# ************* Prepinac -s *****************

if [ "$s_prep" = true ]; then
	for filetype in status stdout stderr; do
		if [ "$REGEX" != 0 ]; then
			set $(find . -name "$filetype"-captured | grep -E "$REGEX" | sort)
		fi

	while [ "$1" != "" ]; do
		mv "$1" "`dirname "$1"`"/"$filetype"-expected
		shift
	done
done
fi

# ************ Prepinac -c ******************

if [ "$c_prep" = true ]; then
	set -e $(find . -name "*-captured")
		while [ "$1" != "" ]; do
			rm -f "$1"
			shift
		done
fi

if [ "$res_failed" = true ]; then
	exit 1
elif [ "$chyba_v" = false ]; then
	exit 0
else 
	exit 1
fi

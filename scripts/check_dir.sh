TOPDIR=$1
SUBDIRS="tests yfs_client yfs_server"

if [ ! -e $TOPDIR/lib ]
	then
		mkdir $TOPDIR/lib
fi

if [ ! -e  $TOPDIR/log ]
	then
		mkdir $TOPDIR/log
fi

for dir in $SUBDIRS
	do
		if [ ! -e $TOPDIR/$dir/bin ]
			then
				mkdir $TOPDIR/$dir/bin
		fi 
	done

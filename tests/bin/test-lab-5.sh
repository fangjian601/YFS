export RPC_LOSSY=$2
for (( i = 0; i < $1 ; i ++ ))
	do
		./lock_server 3772 &
		./lock_tester 3772
		killall lock_server
	done

for i in {1..100}
    do
        echo "Iteration $i. started"
        ./hackrf_sweep-original -f45:245 -w1000000 -1
        echo "Iteration $i. success"
        sleep 1
    done

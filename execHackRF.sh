for i in {1. .3600}
do
echo "Iteration $i started"

./hackrf_sweep -f45:245 -w1000000 -1

echo "Iteration $i success"
done

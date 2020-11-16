./csim     -s 1 -E 1 -b 1 -t traces/yi2.trace
./csim-ref -s 1 -E 1 -b 1 -t traces/yi2.trace

./csim     -s 4 -E 2 -b 4 -t traces/yi.trace
./csim-ref -s 4 -E 2 -b 4 -t traces/yi.trace

./csim     -s 2 -E 1 -b 4 -t traces/dave.trace
./csim-ref -s 2 -E 1 -b 4 -t traces/dave.trace

./csim     -s 2 -E 1 -b 3 -t traces/trans.trace
./csim-ref -s 2 -E 1 -b 3 -t traces/trans.trace

./csim     -s 2 -E 2 -b 3 -t traces/trans.trace
./csim-ref -s 2 -E 2 -b 3 -t traces/trans.trace

./csim     -s 2 -E 4 -b 3 -t traces/trans.trace
./csim-ref -s 2 -E 4 -b 3 -t traces/trans.trace

./csim     -s 5 -E 1 -b 5 -t traces/trans.trace
./csim-ref -s 5 -E 1 -b 5 -t traces/trans.trace

./csim     -s 5 -E 1 -b 5 -t traces/long.trace
./csim-ref -s 5 -E 1 -b 5 -t traces/long.trace
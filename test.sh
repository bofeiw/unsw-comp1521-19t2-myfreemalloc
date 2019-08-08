#!/usr/bin/env bash


# fixed test for test1
#make
#
#
#diff <(./test1 5000) <(echo "+00000 (F, 5000)")  --ignore-all-space
#diff <(./test1 10000) <(echo "+00000 (F,10000)") --ignore-all-space
#diff <(./test1 100) <(echo "+00000 (F, 4096)") --ignore-all-space
#diff <(./test1 5001) <(echo "+00000 (F, 5004)") --ignore-all-space

#./test2 | diff test2exp -

# below is random tes for test2
for rand_n in {1..1}
do
	echo $rand_n

    echo "#define RAND ${rand_n}" > .myHeap.c
    cat test2.c >> .myHeap.c
    gcc .myHeap.c myHeap.c -o .test2

    echo "#define RAND ${rand_n}" > .myHeapExp.c
    echo "#define USE_MYHEAP" >> .myHeapExp.c
    cat test2.c >> .myHeapExp.c
    gcc .myHeapExp.c myHeap.c -o .test2exp

    ./.test2 | diff <(./.test2exp) -
done

rm .myHeap.c*
rm .myHeapExp.c*
rm .test2*
rm .test2exp*

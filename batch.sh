

for file in benchmarks/arithmetic/*.blif
do
    if test -f $file ;
    then
        name=`basename $file`
        filename="${name%%.*}"
        echo filename
        echo "size,delay," > $filename.csv
        echo ${filename} 
        echo "${filename},\c " >> test3.csv
        ./main -f ${file} -i 1000 -r 0.99999 -t 1 -m 1 -c "b;rw;rf;rs" -o $filename.out >> $filename.csv
       # echo "" >> test.csv
    fi
done

for file in benchmarks/random_control/*.blif
do
    if test -f $file ;
    then
        name=`basename $file`
        filename="${name%%.*}"
        echo filename
        echo "size,delay," > $filename.csv
        echo ${filename} 
        echo "${filename},\c " >> test3.csv
        ./main -f ${file} -i 1000 -r 0.99999 -t 1 -m 1 -c "b;rw;rf;rs" -o $filename.out >> $filename.csv
       # echo "" >> test.csv
    fi
done
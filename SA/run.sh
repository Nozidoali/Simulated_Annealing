#make 

echo "file,oldsize,oldlevel,rwrsize,rwrlevel," > test3.csv
for file in ../data/epfl/arithmetic/*.blif
do
    if test -f $file ;
    then
        name=`basename $file`
        filename="${name%%.*}"
        echo ${filename} 
        echo "${filename},\c " >> test3.csv
        ./main -f ${file} -i 10 -r 0.001 -m 0 -o "b;rs;rw;rs;rf;rs;b;rs;rw;rs;rw;rs;b;rs;rf;rs;rw;b" >> test3.csv
       # echo "" >> test.csv
    fi
done

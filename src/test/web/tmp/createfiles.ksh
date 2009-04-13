i=0
date >a.tmp
while [ $i -lt 10000 ]
do
	cp a.tmp file${i}.txt
	i=$((i + 1))
	rm -f $i
done
rm a.tmp

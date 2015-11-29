TARGET=$1
for i in {1..250};
do 
	curl -s  -o /dev/null $TARGET  -w  '%{speed_download}\n' ;
done \
|  awk '{sum += ($1/1024/1024) ; count +=1; } END  {print sum/count*8 }'


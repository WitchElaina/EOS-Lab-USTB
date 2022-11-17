for i in {1..7}  
do  
echo Pushing ex1-$i...;  
cd ex1-$i
pwd
git push https://www.codecode.net/ustb-cs/2020/mszmsz/ex1-$i.git
cd ./..
echo Done!
done  


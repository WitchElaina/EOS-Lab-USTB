git clone https://www.codecode.net/engintime/os-lab/Project-Template/eos-kernel.git
for i in {1..7}  
do  
echo Copy to ex1-$i...
cp ./eos-kernel/*.c ./ex1-$1/
cd ./ex1-$1
git add ./
git commit -m "Add codes"
cd ./..
echo Done.
done
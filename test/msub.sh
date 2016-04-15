nodes=4
min="15:00"

for i in `seq 30`
do
command="msub -l nodes=${nodes} -l walltime=${min} -N hypre${i} ./run.sh"
echo $command
$command
done

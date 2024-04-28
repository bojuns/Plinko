TRACE_DIR=trace_input
binary=${1}
n_warm=${2}
n_sim=${3}
num=${4}
sim=${5}
trace1=`sed -n ''$num'p' sim_list/$sim.txt | awk '{print $1}'`

mkdir -p results_4core
./bin/${binary} -warmup_instructions ${n_warm}000000 -simulation_instructions ${n_sim}000000 -traces ${TRACE_DIR}/${trace1}.champsimtrace.xz 2>&1 | tee results_4core/mix${num}-${binary}-${trace1}.txt

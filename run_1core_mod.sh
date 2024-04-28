TRACE_DIR=trace_input
binary=${1}
n_warm=${2}
n_sim=${3}
num=${4}
sim=${5}
outpath=${6}
max_files=${7}
interval=${8}
geobase=${9}
trace1=`sed -n ''$num'p' sim_list/$sim.txt | awk '{print $1}'`

mkdir -p results_new
./bin/${binary} -warmup_instructions ${n_warm}000000 -simulation_instructions ${n_sim}000000 -traces ${TRACE_DIR}/${trace1}.champsimtrace.xz -snapshot_enable 1 -output_path_base ${outpath} -max_file_count ${max_files} -snapshot_interval ${interval} -geometric_base ${geobase} 2>&1 | tee results_new/mix${num}-${binary}-${trace1}.txt

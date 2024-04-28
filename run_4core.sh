#/usr/bin/env bash

set -x

TRACE_DIR=./trace_input
binary=${1}
n_warm=${2}
n_sim=${3}
num=${4}

trace1=`sed -n "${num}p" sim_list/4core_workloads.txt | awk '{print $1}'`
trace2=`sed -n "${num}p" sim_list/4core_workloads.txt | awk '{print $2}'`
trace3=`sed -n "${num}p" sim_list/4core_workloads.txt | awk '{print $3}'`
trace4=`sed -n "${num}p" sim_list/4core_workloads.txt | awk '{print $4}'`

mkdir -p results_4core_new
# (./bin/${binary} -warmup_instructions ${n_warm}000000 -simulation_instructions ${n_sim}000000 -traces ${TRACE_DIR}/${trace1}.champsimtrace.xz ${TRACE_DIR}/${trace2}.champsimtrace.xz ${TRACE_DIR}/${trace3}.champsimtrace.xz ${TRACE_DIR}/${trace4}.champsimtrace.xz) &> results_4core/mix${num}-${binary}.txt648.exchange2_s-1227B

./bin/${binary} -warmup_instructions ${n_warm}000000 -simulation_instructions ${n_sim}000000 -traces ${TRACE_DIR}/${trace1}.champsimtrace.xz ${TRACE_DIR}/${trace2}.champsimtrace.xz ${TRACE_DIR}/${trace3}.champsimtrace.xz ${TRACE_DIR}/${trace4}.champsimtrace.xz 2>&1 | tee results_4core_new/mix${num}-${binary}.txt
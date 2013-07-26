#!/bin/bash

TESTNAME=sample

. lib/setup_generic.sh
. lib/setup_test_core.sh
. lib/setup_sample_tools.sh
. lib/setup_sample_test.sh

do_test 'sample test' "${TESTPROG} -m private -n1 -p ${PIPE}" control_sample check_sample
do_test 'sample test' "${TESTPROG} -h 2048 -m private -n1 -p ${PIPE}" control_sample check_sample
do_test_async 'sample async test' control_sample_async check_sample_async

show_summary
exit 0

#!/bin/bash

if [[ "$0" =~ "$BASH_SOURCE" ]] ; then
    echo "$BASH_SOURCE should be included from another script, not directly called."
    exit 1
fi

BASEVFN=0x700000000

# Set global variable to the path of your test helper programs
PAGETYPES=${LDIR}/page-types
[ ! -x "$PAGETYPES" ] && echo "${PAGETYPES} not found." >&2 && exit 1
MCEINJECT=${LDIR}/mceinj.sh
[ ! -x "$MCEINJECT" ] && echo "${MCEINJECT} not found." >&2 && exit 1

# Define caller of your test helper programs
# Here is an example of hwpoison tools
all_unpoison() { ${PAGETYPES} -b hwpoison,compound_tail=hwpoison -x -N; }
HWCORRUPTED=()
get_HWCorrupted() { grep "HardwareCorrupted" /proc/meminfo | tr -s ' ' | cut -f2 -d' '; }
get_nr_corrupted() {
    [ "$1" -eq  1 ] && HWCORRUPTED=()
    HWCORRUPTED[$1]=$(get_HWCorrupted)
}
show_nr_corrupted() {
    echo -n "HWCorrupted pages, "
    for i in 1 2 3 ; do
        echo -n "$i:${HWCORRUPTED[$i]} "
    done
    echo ""
}
check_nr_hwcorrupted() {
    count_testcount
    if [ "${HWCORRUPTED[1]}" -ne "${HWCORRUPTED[2]}" ] && [ "${HWCORRUPTED[1]}" -eq "${HWCORRUPTED[3]}" ] ; then
        count_success "accounting \"HardwareCorrupted\" back to original value (${HWCORRUPTED[1]} -> ${HWCORRUPTED[2]} -> ${HWCORRUPTED[3]})"
    else
        count_failure "accounting \"HardwareCorrupted\" not back to original value (${HWCORRUPTED[1]} -> ${HWCORRUPTED[2]} -> ${HWCORRUPTED[3]})"
    fi
}

# Checking test specific requirement for kernel module / rpm packages
! lsmod | grep mce_inject      > /dev/null && modprobe mce_inject
! lsmod | grep hwpoison_inject > /dev/null && modprobe hwpoison_inject
! which expect > /dev/null && \
    echo "You need to install expect to run this test." >&2 && exit 1

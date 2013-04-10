#!/bin/bash

SDIR=`dirname $BASH_SOURCE`
PID=""
PFN=""
ERRORTYPE=""
DOUBLE=false
VERBOSE=false
TARGET=
while getopts "p:a:e:Dv" opt ; do
    case $opt in
        p) PID=$OPTARG ;;
        a) PFN=$OPTARG ;;
        e) ERRORTYPE=$OPTARG ;;
        D) DOUBLE=true ;;
        v) VERBOSE=true ;;
        *) usage 0 ;;
    esac
done

usage() {
    local sname=`basename $BASH_SOURCE`
    echo "Usage: $sname [-p pid] -a pfn -e errortype [-Dv]"
    echo ""
    echo "Options:"
    echo "  -p: set process ID of the target process."
    echo "  -a: set memory address (in page unit). If -p option is given,"
    echo "      the given address is virtual one. Otherwise it's physical one."
    echo "  -e: set error type to be injected. It's one of the following:"
    echo "      mce-ce, mce-srao, hard-offline, soft-offline"
    echo "  -D: inject twice"
    echo "  -v: verbose"
    exit $1
}

inject_error() {
    local tmpf=`mktemp`

    if [ "$ERRORTYPE" = "hard-offline" ] ; then
        echo "Hard offlining host pfn ${TARGET}"
        echo ${TARGET}000 > /sys/devices/system/memory/hard_offline_page
    elif [ "$ERRORTYPE" = "soft-offline" ] ; then
        echo "Soft offlining host pfn ${TARGET}"
        echo ${TARGET}000 > /sys/devices/system/memory/soft_offline_page
    elif [ "$ERRORTYPE" = "mce-srao" ] ; then
        echo "Injecting MCE on host pfn ${TARGET}"
        cat <<EOF > ${tmpf}.mce-inject
CPU `cat /proc/self/stat | cut -d' ' -f39` BANK 2
STATUS UNCORRECTED SRAO 0x17a
MCGSTATUS RIPV MCIP
ADDR ${TARGET}000
MISC 0x8c
RIP 0x73:0x1eadbabe
EOF
        mce-inject ${tmpf}.mce-inject
    elif [ "$ERRORTYPE" = "mce-ce" ] ; then
        echo "Injecting Corrected Error on host pfn ${TARGET}"
        cat <<EOF > ${tmpf}.mce-inject
CPU `cat /proc/self/stat | cut -d' ' -f39` BANK 2
STATUS CORRECTED 0xc0
ADDR ${TARGET}000
EOF
        mce-inject ${tmpf}.mce-inject
    else
        echo "undefined injection type [$ERRORTYPE]. Abort"
        return 1
    fi
    rm -rf ${tmpf:?DANGER}*
    return 0
}

if [[ ! "$ERRORTYPE" =~ (mce-srao|mce-ce|hard-offline|soft-offline) ]] ; then
    echo "-e <ERRORTYPE> should be given."
    exit 1
fi

if [ ! "$PFN" ] ; then
    echo "-a <PFN> should be given."
    exit 1
fi

if [ "$PID" ] ; then
    TARGET=0x$(ruby -e 'printf "%x\n", IO.read("/proc/'$PID'/pagemap", 0x8, '$PFN'*8).unpack("Q")[0] & 0xfffffffffff')
    echo "Injecting MCE to local process (pid:$PID) at vfn:$PFN, pfn:$TARGET"
else
    TARGET="$PFN"
    echo "Injecting MCE to physical address pfn:$TARGET"
fi
inject_error $ERRORTYPE $TARGET 2>&1
[ "$DOUBLE" = true ] && inject_error $ERRORTYPE $TARGET 2>&1

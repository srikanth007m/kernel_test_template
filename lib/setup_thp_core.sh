#!/bin/bash

SYS_THPDIR=/sys/kernel/mm/transparent_hugepage
[ ! -d ${SYS_THPDIR} ] && die "THP not supported in your kernel"
echo "always" > ${SYS_THPDIR}/enabled

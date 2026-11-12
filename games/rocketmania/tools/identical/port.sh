#!/bin/bash
# port.sh <out dir> <port arguments...>: the port under rundesk (invisible, silent, low priority) with -fast, on a fresh
# user dir inside the out dir (KEEP_USER=1 keeps the one a run before left). Its output goes to <out dir>/out.txt.
source "$(dirname "$0")/common.sh"
D="$1"; shift
mkdir -p "$D"; [ -n "$KEEP_USER" ] || rm -rf "$D/user"
# RM_REG="Name=value,Name=value": settings the run starts with (the emulated registry; the original gets them through
# orig.sh from HKCU, --reg-map popcap)
if [ -n "$RM_REG" ]; then
	mkdir -p "$D/user"
	{ echo "; Settings (emulated Windows registry, HKEY_CURRENT_USER)"; echo; echo '[SOFTWARE\PopCap\RocketMania]'
	  for kv in ${RM_REG//,/ }; do echo "${kv%%=*}=dword:${kv#*=}"; done; } > "$D/user/registry.ini"
fi
"$RUNDESK" "$(win "${PORT_EXE:-build/native/rocketmania.exe}")" -userdir="$(win "$D/user")" -fast -drawevery "$@" > "$D/out.txt" 2>&1
echo "exit $?"

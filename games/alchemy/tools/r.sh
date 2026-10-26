#!/bin/bash
# r.sh <addr>...: print normalized decompilation without exception-handling noise, 130 columns
python games/alchemy/tools/norm.py ${FIELDS:-re/alchemy/fields_Board.txt} "$@" 2>&1 | cut -c1-130 | grep -v "_Tidy\|^ *$\|unaff_EBP + -4) = \|^ *\* *$\|ExceptionList\|operator_new" | grep -v 'string_cstr[^"]*$'

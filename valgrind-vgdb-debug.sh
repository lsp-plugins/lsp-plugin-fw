#!/bin/bash

valgrind \
    --vgdb-error=0 \
    --vgdb=yes \
    --tool=memcheck \
    --leak-check=full \
    Debug/lsp-plugin-fw mtest --debug --nofork standalone --args phase_detector


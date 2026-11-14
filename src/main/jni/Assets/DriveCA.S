.section .rodata
.balign 16
.global drive_ca_start
.hidden drive_ca_start
.global drive_ca_end
.hidden drive_ca_end
drive_ca_start:
.incbin DRIVE_CA_FILE
drive_ca_end:
.section .note.GNU-stack,"",%progbits

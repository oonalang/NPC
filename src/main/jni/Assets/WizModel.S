.section .rodata
.balign 16
.global wiz_model_start
.hidden wiz_model_start
wiz_model_start:
.incbin WIZ_MODEL_FILE
.global wiz_model_end
.hidden wiz_model_end
wiz_model_end:
.section .note.GNU-stack,"",%progbits

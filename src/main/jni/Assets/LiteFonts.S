.section .rodata
.balign 16
.global lite_body_start
.hidden lite_body_start
lite_body_start:
.incbin LITE_BODY_FILE
.global lite_body_end
.hidden lite_body_end
lite_body_end:
.balign 16
.global lite_title_start
.hidden lite_title_start
lite_title_start:
.incbin LITE_TITLE_FILE
.global lite_title_end
.hidden lite_title_end
lite_title_end:
.balign 16
.global lite_icons_start
.hidden lite_icons_start
lite_icons_start:
.incbin LITE_ICONS_FILE
.global lite_icons_end
.hidden lite_icons_end
lite_icons_end:
.section .note.GNU-stack,"",%progbits

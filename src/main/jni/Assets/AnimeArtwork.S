.section .rodata
.balign 16
.global anime_artwork_start
.hidden anime_artwork_start
anime_artwork_start:
.incbin ANIME_ARTWORK_FILE
.global anime_artwork_end
.hidden anime_artwork_end
anime_artwork_end:
.section .note.GNU-stack,"",%progbits

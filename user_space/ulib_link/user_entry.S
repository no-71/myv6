.section the_user_start
.global user_start

user_start:
    call main
    call exit

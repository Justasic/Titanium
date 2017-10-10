# Find our multiplexer, choose the best one possible.
if (HAVE_SYS_EPOLL_H)
	set(CHILD_SCOPE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/multiplexer_epoll.cpp PARENT_SCOPE)
else (HAVE_SYS_EPOLL_H)
    # Okay, so this system doesn't have epoll, let's try kqueue.
    if (HAVE_KQUEUE)
        set(CHILD_SCOPE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/multiplexer_kqueue.cpp PARENT_SCOPE)
    else (HAVE_KQUEUE)
        # Okay, we don't have kqueue, lets try poll.
        if (HAVE_POLL)
            set(CHILD_SCOPE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/multiplexer_poll.cpp PARENT_SCOPE)
        else (HAVE_POLL)
            # alright, we'll give in and try select.
            if (HAVE_SELECT)
                set(CHILD_SCOPE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/multiplexer_select.cpp PARENT_SCOPE)
            else (HAVE_SELECT)
                # wtf? are we on MS-DOS?
                message(FATAL_ERROR "Cannot find a compatible multiplexer for this platform, please file a bug.")
            endif (HAVE_SELECT)
        endif(HAVE_POLL)
    endif(HAVE_KQUEUE)
endif (HAVE_SYS_EPOLL_H)

# ****************************************************************************
# * Sven Lukas
# *
# * (C) COPYRIGHT Sven Lukas 2004, 2010
# ****************************************************************************
# * ZSystem
# ****************************************************************************


# ============================================================================
# = Define BASE and TOP variable
# ============================================================================
ifndef BASE
  BASE = $(shell while ! test -e Makefile.project; do cd ..  ; done; pwd)
  export BASE
endif
ifndef TOP
  TOP = $(shell while ! test -e Makefile.post; do cd ..  ; done; pwd)
  export TOP
endif



# ============================================================================
# = Inlude Makefile.pre
# = --------------------------------------------------------------------------
# = Definiton of
# = - compiler, flags, binaries and variables
# = - targets: all (calls all_post)
# =            Makefile.dep
# = - includes: Makefile.dep
# ============================================================================
include $(TOP)/Makefile.pre

# ============================================================================
# = Inlude Makefile.project
# = --------------------------------------------------------------------------
# = Definiton of specific settings for the whole project like include path
# ============================================================================
include $(BASE)/Makefile.project



#SUBDIRS    += cpp

all_post:



# ============================================================================
# = Inlude Makefile.post
# ============================================================================
include $(TOP)/Makefile.post

MODULES = json 
OBJS = json.o 
DATA_built = json.sql
DATA = uninstall_json.sql
DOCS = README.txt

ifdef USE_PGXS
PGXS = $(shell pg_config --pgxs)
include $(PGXS)
else
subdir = contrib/json 
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

SHLIB_LINK = -L/$(TOOLCHAIN_DIR)/installed/lib 

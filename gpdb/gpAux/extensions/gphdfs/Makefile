MODULE_big = gphdfs
OBJS       = gphdfsformatter.o gphdfsprotocol.o

REGRESS = basic privileges

PG_CPPFLAGS = -I$(libpq_srcdir)
PG_LIBS = $(libpq_pgport)

ANT = ant
ANT_OPTS="-Djavax.net.ssl.trustStore=../../releng/make/dependencies/cacerts"

PG_CONFIG := pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

JAR_FILES = cdh4.1-gnet-1.2.0.0.jar

all:

all_jar: $(JAR_FILES) 

unittest: $(JAR_FILES)
	$(ANT) clean
	ANT_OPTS=$(ANT_OPTS) $(ANT) test -Dorg=apache \
                -Dname=hadoop-core \
                -Drevision=2.0.0-mr1-cdh4.1.2 \
                -Dhadoop-common.revision=2.0.0-cdh4.1.2 \
                -Dgphdgnet.version=cdh4.1-gnet-1.2.0.0 \
                -Dgpgnet.src=1.2 \
                -Dgpgnet.configuration=ut

install-jars: $(JAR_FILES)
	$(INSTALL_PROGRAM) $(JAR_FILES) '$(DESTDIR)$(libdir)/hadoop/'
	$(INSTALL_PROGRAM) hadoop_env.sh$(X) '$(DESTDIR)$(libdir)/hadoop/hadoop_env.sh$(X)'

.PHONY: install-gphdfs.so
install-gphdfs.so: gphdfs.so
	$(INSTALL_PROGRAM) gphdfs.so $(DESTDIR)$(pkglibdir)

.PHONY: install-data
install-data:
	$(INSTALL_DATA) gphdfs.sql '$(DESTDIR)$(datadir)/cdb_init.d/gphdfs.sql'

install: install-jars install-gphdfs.so install-data

installdirs:
	$(MKDIR_P) '$(DESTDIR)$(libdir)/hadoop'
	$(MKDIR_P) '$(DESTDIR)$(docdir)/javadoc'

uninstall:
	rm -f '$(DESTDIR)$(bindir)/*.jar'
	rm -f '$(DESTDIR)$(bindir)/hadoop_env.sh$(X)'
	rm -f '$(DESTDIR)$(datadir)/cdb_init.d/gphdfs.sql'

clean: clean-extras

clean-extras:
	$(ANT) clean
	rm -rf *-javadoc
	rm -f *-javadoc.tar
	rm -rf result

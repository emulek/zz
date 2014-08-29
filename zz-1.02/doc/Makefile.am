SUBDIRS =

@AMDEP_TRUE@@am__include@ @am__quote@generated_files.mk@am__quote@

pictures = apicture.png
source_tarball_files = $(PACKAGE).xsl $(PACKAGE).xml chapter1.xml chapter2.xml chapter3.xml

htmlhelpdir = $(helpdir)
htmlhelp_created_files = $(PACKAGE).hhc $(PACKAGE).hhk $(PACKAGE).hhp $(TESTPROJ_DOCFILES)
htmlhelp_DATA = $(pictures) $(TESTPROJ_DOCFILES)

EXTRA_DIST = $(source_tarball_files) $(pictures) generated_files.mk $(htmlhelp_created_files)

DISTCLEANFILES = Makefile

check_can_build_documentation:
	@if test x$(can_build_documentation) != "xyes" ; then \
		echo "Missing libxslt." \
		"Rerun configure and check for 'checking whether documentation can be changed and compiled... yes'!" ; \
		exit 1 ; \
	fi

generated_files_include:
	@echo -n "TESTPROJ_DOCFILES = "> $(srcdir)/generated_files.mk2 
	@find . -maxdepth 1 -type f -name "*.html" | sort | $(AWK) '{printf(" \\\n\t%s", $$0)}' \
		>> $(srcdir)/generated_files.mk2
	@if diff $(srcdir)/generated_files.mk $(srcdir)/generated_files.mk2 >/dev/null 2>&1 ; then \
		echo "generated_files.mk is unchanged" ; \
		rm -f $(srcdir)/generated_files.mk2 ; \
	else \
		mv $(srcdir)/generated_files.mk2 $(srcdir)/generated_files.mk ; \
	fi

all: $(htmlhelp_created_files)

force-update-doc: doc-clean $(htmlhelp_created_files)

$(htmlhelp_created_files): $(source_tarball_files)
	@ $(MAKE) check_can_build_documentation
	$(MAKE) doc-clean
	$(xsltproc) $(srcdir)/$(PACKAGE).xsl $(srcdir)/$(PACKAGE).xml
	@echo "creating generated_files.mk"
	$(MAKE) generated_files_include

#maintainer-clean: doc-clean maintainer-clean-recursive

doc-clean:
	-rm -f *.hhp
	-rm -f *.html
	-rm -f *.hhk
	-rm -f *.hhc

.PHONY: check_can_build_documentation generated_files_include force-update-doc doc-clean


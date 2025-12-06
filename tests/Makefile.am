TESTS = test-list test-extract test-print test-create test-combined-flags test-delete

check_SCRIPTS = $(TESTS)

EXTRA_DIST = $(TESTS) recipes.brarchive

# Test scripts need to find the built executable
TESTS_ENVIRONMENT = \
	PATH=$(abs_top_builddir)/src:$$PATH \
	TOOL_NAME=$(TOOL_NAME) \
	TOOL_BINARY=$(abs_top_builddir)/src/br_ar \
	TEST_SRCDIR=$(abs_srcdir) \
	TEST_BUILDDIR=$(abs_builddir)

# Clean up test artifacts
CLEANFILES = test_output test_archive.brarchive test_extract_dir


# ========================================================================
#
#  rules.mk
#
#  Author: Patric Jensfelt
#
#  Changes by Pablo d'Angelo
#   - removed CORBA stuff
#   - added LIBS, APPS and TESTS expansion (written by Boris Kluge)
#   - make static instead of shared libraries
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# ========================================================================

#=============================================================================
# When you just type "make", the stuff listed after all: below will be done
#=============================================================================

all: depend lib apps

#=============================================================================
# To make it possible to both do what has to be done in this directory
# and in the subdirectories we define nested and local rules. The
# nested rules are used to pass on the target to the subdirectory and
# the local are the ones that are used in this directory. That is, for
# each rule like links there will be a links.nested and a
# links.local. So for each target the nested rule will run first which
# means that a "make links" will first lead to links being done in the
# subdirectories (links.nested) and then in the current directory
# (links.local)
#=============================================================================
TARGETS = check depend lib apps tests clean distclean depclean
TARGETS_NESTED = $(TARGETS:%=%.nested)
TARGETS_LOCAL = $(TARGETS:%=%.local)
#check: check.nested check.local
depend: depend.nested depend.local
lib: lib.nested lib.local
#slibs: slibs.nested slibs.local
apps: apps.nested apps.local
tests: tests.nested tests.local
#install: install.nested install.local
clean: clean.nested clean.local
distclean: distclean.nested distclean.local
depclean: depclean.nested depclean.local
docs:
	doxygen
#=============================================================================
# All nested targets are taken care of by this rule. It simply goes
# through the subdirectories and issues a make with the target name
# (which might be links.nested) with the .nested removed. This gives
# the effect that all the command that you type will be sent first to
# the subdirectories and then treated locally.
#=============================================================================
$(TARGETS_NESTED):
#	@echo "Dealing with target $@"
	@for subdir in $(SUBDIRS) __dummy__dir__ ; do \
            if test -d $$subdir; then \
	      echo "Descending directory $$subdir to do \"make $(@:%.nested=%)\""; \
              $(MAKE) -C $$subdir $(@:%.nested=%); \
            fi; \
        done

#=============================================================================
# .PHONY: A standard safety precaution if you should create a file with the
#         same name as a target.
#=============================================================================
.PHONY : $(TARGETS) docs


#=============================================================================
# expand LIB, APP and TEST targets
#=============================================================================


OBJ_FILES =     $(foreach lib, $(LIBS), $(LIB_$(lib)_OBJ)) \
                $(foreach app, $(APPS), $(APP_$(app)_OBJ)) \
                $(foreach test, $(TESTS), $(TEST_$(test)_OBJ))

LIBS_BIN =      $(patsubst %, lib%.a, $(foreach lib, $(LIBS), $(LIB_$(lib)_BIN)))
APPS_BIN =      $(foreach app, $(APPS), $(APP_$(app)_BIN))
TESTS_BIN =     $(foreach test, $(TESTS), $(TEST_$(test)_BIN))

#=============================================================================
# Build dependencies
#=============================================================================
depend.local: Makefile.depend Makefile

Makefile.depend: $(DEPEND_FILES) Makefile
	makedepend $(CXXFLAGS) $(CFLAGS) $(EXTRA_INC_FLAGS) -p $(OBJ_DIR)/ -DDEPEND -f- $(DEPEND_FILES) > Makefile.depend 2>/dev/null
#	makedepend $(CXXFLAGS) $(CFLAGS) $(EXTRA_INC_FLAGS) -a -DDEPEND -f Makefile.depend $(DEPEND_FILES_H) 2>/dev/null

#=============================================================================
# Create libraries
#=============================================================================

lib.local: $(OBJ_DIR) $(LIB_DIR) $(addprefix $(LIB_DIR)/, $(LIBS_BIN))
	echo $(OBJ_FILES)

#=============================================================================
# Create Applicatios
#=============================================================================

apps.local: $(OBJ_DIR) $(BIN_DIR) $(APPS_BIN)

#=============================================================================
# Create test Applications use "make tests" to build these tests
#=============================================================================

tests.local: $(OBJ_DIR) $(TESTS_BIN)

#=============================================================================
# Install links to the binary files
#=============================================================================

install.local: $(APPS)
	@[ -d $(BIN_DIR) ] || $(MKDIR) $(BIN_DIR);
	@for b in $(basename $(APPS)) __dummy__file__ ; do \
		if test -f $$b; then \
		   ln -s -f `pwd`/$$b $(BIN_DIR)/$$b;\
		fi; \
	done

#=============================================================================
# General rules for how to build object files
#=============================================================================


$(OBJ_DIR)/%.o %.o: %.c
	$(ECHO) "    ---- Compiling $< (C)"
	$(SILENT) $(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJ_DIR)/%.o %.o: %.cpp
	$(ECHO) "    ---- Compiling $< (C++)"
	$(SILENT) $(CXX) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJ_DIR)/%.s %.s: %.c
	$(ECHO) "    ---- Creating assembly file for $< (C)"
	$(SILENT) $(CC) $(CFLAGS) $(INCLUDES) -S $<

$(OBJ_DIR)/%.s: %.cpp
	$(ECHO) "    ---- Creating assembly file for $< (C++)"
	$(SILENT) $(CXX) $(CFLAGS) $(INCLUDES) -S $<

$(OBJ_DIR)/%.c %.c: %.l
	$(ECHO) "    ---- Creating C file for $< (LEX)"
	$(SILENT) $(LEX) $<
	$(SILENT) mv -f lex.yy.c $*.c

$(OBJ_DIR)/%.C %.C: %.l
	$(ECHO) "    ---- Creating C++ file for $< (LEX)"
	$(SILENT) $(LEX) -+ $<
	$(SILENT) mv -f lex.yy.cc $*.cc
	$(SILENT) mv -f FlexLexer.h $*.h

$(OBJ_DIR)/%.cc %.cc: %.l
	$(ECHO) "    ---- Creating C++ file for $< (LEX)"
	$(SILENT) $(LEX) -+ $<
	$(SILENT) mv -f lex.yy.c $*.cc
	$(SILENT) mv -f FlexLexer.h $*.h

$(OBJ_DIR)/%.tab.c %.tab.c: %.y
	$(ECHO) "    ----- Creating C file for $< (YACC)"
	$(SILENT) $(YACC) -b $* $<

$(OBJ_DIR)/%.tab.C %.tab.C: %.y
	$(ECHO) "    ----- Creating C++ file for $< (YACC)"
	$(SILENT) $(YACC) -b $* $<
	$(SILENT) mv -f $*.tab.c $*.tab.cc

$(OBJ_DIR)/%.tab.cc %.tab.cc: %.y
	$(ECHO) "    ---- Creating C++ file for $< (YACC)"
	$(SILENT) $(YACC) -b $* $<
	$(SILENT) mv -f $*.tab.c $*.tab.cc
	$(CXX) $(CXXFLAGS) $(CFLAGS) $(EXTRA_INC_FLAGS) -shared $< -o $@


#=============================================================================
# Rules to build APP LIB and TEST targets
#=============================================================================

$(addprefix $(LIB_DIR)/,$(LIBS_BIN)): %: $(addprefix $(OBJ_DIR)/,$(foreach i, $(LIBS), $(LIB_$(i)_OBJ)))
	$(foreach i, $(LIBS), \
	  $(if $(findstring lib$(LIB_$(i)_BIN).a,$@), \
	    $(if $(filter $(addprefix $(OBJ_DIR)/,$(LIB_$(i)_OBJ)),$?), \
	      $(SILENT) $(AR) $@ $(addprefix $(OBJ_DIR)/, $(LIB_$(i)_OBJ)) \
	    ) \
	  ) \
	)
	$(foreach i, $(LIBS), \
	  $(if $(findstring lib$(LIB_$(i)_BIN).a,$@), \
	    $(if $(filter $(addprefix $(OBJ_DIR)/,$(LIB_$(i)_OBJ)),$?), \
	      $(SILENT) $(RANLIB) $@ \
	    ) \
	  ) \
	)

$(APPS_BIN): %: $(addprefix $(LIB_DIR)/,$(LIB_NAMES)) $(addprefix $(OBJ_DIR)/,$(foreach i, $(APPS), $(APP_$(i)_OBJ)))
	$(foreach i, $(APPS), \
	  $(if $(findstring $(APP_$(i)_BIN),$@), \
	    $(if $(filter $(addprefix $(LIB_DIR)/,$(LIB_NAMES)) $(addprefix $(OBJ_DIR)/,$(APP_$(i)_OBJ)),$?), \
	      $(SILENT)$(CXX) $(INCLUDES) $(LIB_DIRS) -o $@ $(addprefix $(OBJ_DIR)/,$(APP_$(i)_OBJ)) $(APP_$(i)_LFLAGS) \
	    ) \
	  ) \
	)

$(TEST_BIN): %: $(addprefix $(LIB_DIR)/,$(LIB_NAMES)) $(addprefix $(OBJ_DIR)/,$(foreach i, $(TESTS), $(TEST_$(i)_OBJ)))
	$(foreach i, $(TESTS), \
	  $(if $(findstring $(TEST_$(i)_BIN),$@), \
	    $(if $(filter $(addprefix $(LIB_DIR)/,$(LIB_NAMES)) $(addprefix $(OBJ_DIR)/,$(TEST_$(i)_OBJ)),$?), \
	      $(SILENT)$(CXX) $(INCLUDES) $(LIB_DIRS) -o $@ $(addprefix $(OBJ_DIR)/,$(TEST_$(i)_OBJ)) $(TEST_$(i)_LFLAGS) \
	    ) \
	  ) \
	)

# This is fun, isn't it? ;-)
#
# These rules are complicated since we want to build a dynamic number of targets
# with a static number of rules. The structure is as follows:
#
# [target set]: %: [requirement set]
#       FOR EACH possible target DO
#         IF this is the target that shall be built THEN
#           IF this target depends on some files that have actually changed ($?) THEN
#             build it
#           ENDIF
#         ENDIF
#       DONE


#=============================================================================
# rules to make the directories
#=============================================================================

$(OBJ_DIR) $(LIB_DIR) $(BIN_DIR) $(TST_DIR):
	$(SILENT) $(MKDIR) $@

#=============================================================================
# Things to remove when you type make clean
#=============================================================================

TO_REMOVE1 = $(addprefix $(OBJ_DIR)/,$(OBJ_FILES))
clean.local:
	@echo Removing files $(TO_REMOVE1)
	@rm -f $(TO_REMOVE1)

#=============================================================================
# Things to remove when you type make distclean
#=============================================================================

TO_REMOVE2 = $(addprefix $(OBJ_DIR)/,$(OBJ_FILES)) \
	     Makefile.depend *~ \
	     $(addprefix $(LIB_DIR)/, $(LIBS_BIN)) \
	     $(addprefix $(BIN_DIR)/, $(APPS_BIN)) \
	     $(addprefix $(TST_DIR)/, $(TESTS_BIN))

distclean.local:
	@echo Removing files $(TO_REMOVE2)
	@rm -f $(TO_REMOVE2)
	@echo Removing directory $(OBJ_DIR)
	@-rmdir $(OBJ_DIR)

#=============================================================================
# Things to remove when you type "make depclean"
#=============================================================================

depclean.local:
	@echo Removing Makefile.depend
	$(SILENT)rm -f Makefile.depend


#=============================================================================
# Include dependencies
#=============================================================================


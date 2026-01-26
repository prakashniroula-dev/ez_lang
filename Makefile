# Compiler settings
CC = gcc
CXXFLAGS = -std=c11 -Wall -Iinclude
LDFLAGS = 

APPNAME = ezc
EXT = .c
SRCDIR = src
LIBDIR = lib
OBJDIR = obj

# Collect all source files
SRC = $(wildcard $(SRCDIR)/*$(EXT)) \
      $(wildcard $(LIBDIR)/*$(EXT)) \
      $(wildcard $(LIBDIR)/*/*$(EXT))

# Object files
OBJ = $(patsubst $(SRCDIR)/%$(EXT),$(OBJDIR)/%.o,$(wildcard $(SRCDIR)/*$(EXT)))
OBJ += $(patsubst $(LIBDIR)/%$(EXT),$(OBJDIR)/lib/%.o,$(wildcard $(LIBDIR)/*$(EXT)))
OBJ += $(patsubst $(LIBDIR)/%/%$(EXT),$(OBJDIR)/lib/%/%.o,$(wildcard $(LIBDIR)/*/*$(EXT)))

# Dependencies
DEP = $(OBJ:.o=.d)

# OS-specific cleanup
RM = rm
DELOBJ = $(OBJ)

# Targets
all: $(APPNAME)

$(APPNAME): $(OBJ)
	$(CC) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Object file rules
$(OBJDIR)/%.o: $(SRCDIR)/%$(EXT)
	@mkdir -p $(dir $@)
	$(CC) $(CXXFLAGS) -o $@ -c $<

$(OBJDIR)/lib/%.o: $(LIBDIR)/%$(EXT)
	@mkdir -p $(dir $@)
	$(CC) $(CXXFLAGS) -o $@ -c $<

$(OBJDIR)/lib/%/%.o: $(LIBDIR)/%/%$(EXT)
	@mkdir -p $(dir $@)
	$(CC) $(CXXFLAGS) -o $@ -c $<

# Dependencies
%.d: %.c
	@$(CC) $(CXXFLAGS) -MM -MT $(@:.d=.o) $< > $@

-include $(DEP)

# Clean
.PHONY: clean
clean:
	$(RM) -rf $(DELOBJ) $(DEP) $(APPNAME)

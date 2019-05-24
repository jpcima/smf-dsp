_OBJS := $(SOURCES:%=obj/$(notdir $(PLUGIN))/%.o)

all: $(PLUGIN)
clean: clean-plugin-$(PLUGIN)

clean-plugin-$(PLUGIN): PLUGIN := $(PLUGIN)
clean-plugin-$(PLUGIN):
	rm -f $(PLUGIN)
	rm -rf obj/$(notdir $(PLUGIN))
.PHONY: clean-plugin-$(PLUGIN)

$(PLUGIN): $(_OBJS)
	$(call link_cxx,-shared)
$(filter %.c.o,$(_OBJS)): obj/$(notdir $(PLUGIN))/%.c.o: %.c
	$(call compile_c,-fPIC)
$(filter %.cc.o,$(_OBJS)): obj/$(notdir $(PLUGIN))/%.cc.o: %.cc
	$(call compile_cxx,-fPIC)
$(filter %.cpp.o,$(_OBJS)): obj/$(notdir $(PLUGIN))/%.cpp.o: %.cpp
	$(call compile_cxx,-fPIC)

-include $(_OBJS:%.o=%.d)

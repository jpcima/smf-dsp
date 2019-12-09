_OBJS := $(SOURCES:%=obj/$(notdir $(PLUGIN))/%.o)

all: $(PLUGIN)
clean: clean-plugin-$(PLUGIN)

clean-plugin-$(PLUGIN): PLUGIN := $(PLUGIN)
clean-plugin-$(PLUGIN):
	rm -f $(PLUGIN)
	rm -rf obj/$(notdir $(PLUGIN))
.PHONY: clean-plugin-$(PLUGIN)

$(PLUGIN): $(_OBJS)
	$(call link_cxx,$(if $(NOT_WASM),-shared,-s SIDE_MODULE=1))
$(filter %.c.o,$(_OBJS)): obj/$(notdir $(PLUGIN))/%.c.o: %.c
	$(call compile_c,$(if $(NOT_WASM),-fPIC,-s SIDE_MODULE=1))
$(filter %.cc.o,$(_OBJS)): obj/$(notdir $(PLUGIN))/%.cc.o: %.cc
	$(call compile_cxx,$(if $(NOT_WASM),-fPIC,-s SIDE_MODULE=1))
$(filter %.cpp.o,$(_OBJS)): obj/$(notdir $(PLUGIN))/%.cpp.o: %.cpp
	$(call compile_cxx,$(if $(NOT_WASM),-fPIC,-s SIDE_MODULE=1))

-include $(_OBJS:%.o=%.d)

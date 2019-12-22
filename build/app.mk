_OBJS := $(SOURCES:%=obj/$(notdir $(APP))/%.o)

all: $(APP)
clean: clean-app-$(APP)

clean-app-$(APP): APP := $(APP)
clean-app-$(APP):
	rm -f $(APP)
	rm -rf obj/$(notdir $(APP))
	rm -rf gen/$(notdir $(APP))
.PHONY: clean-app-$(APP)

$(APP): $(_OBJS)
	$(call link_cxx)
$(filter %.c.o,$(_OBJS)): obj/$(notdir $(APP))/%.c.o: %.c
	$(call compile_c)
$(filter %.cc.o,$(_OBJS)): obj/$(notdir $(APP))/%.cc.o: %.cc
	$(call compile_cxx)
$(filter %.cpp.o,$(_OBJS)): obj/$(notdir $(APP))/%.cpp.o: %.cpp
	$(call compile_cxx)

-include $(_OBJS:%.o=%.d)

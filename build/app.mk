_OBJS := $(SOURCES:%=obj/%.o)

all: $(APP)
clean: clean-app-$(APP)

clean-app-$(APP):
	rm -f $(APP)
	rm -rf obj/
.PHONY: clean-app-$(APP)

$(APP): $(_OBJS)
	$(call link_cxx)
$(filter %.c.o,$(_OBJS)): obj/%.c.o: %.c
	$(call compile_c)
$(filter %.cc.o,$(_OBJS)): obj/%.cc.o: %.cc
	$(call compile_cxx)
$(filter %.cpp.o,$(_OBJS)): obj/%.cpp.o: %.cpp
	$(call compile_cxx)

-include $(_OBJS:%.o=%.d)

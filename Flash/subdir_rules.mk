################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
AIO.obj: ../AIO.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: C2000 Compiler'
	"c:/ti/ccsv6/tools/compiler/c2000_6.2.10/bin/cl2000" -v28 -ml -mt -O0 --include_path="c:/ti/ccsv6/tools/compiler/c2000_6.2.10/include" --include_path="C:/ti/controlSUITE/device_support/f2802x/v220/f2802x_common/lib" --include_path="C:/ti/controlSUITE/development_kits/C2000_LaunchPad" -g --define="_FLASH" --define=NDEBUG --define="_DEBUG" --define="LARGE_MODEL" --quiet --verbose_diagnostics --diag_warning=225 --gen_func_subsections=on --output_all_syms --cdebug_asm_data --preproc_with_compile --preproc_dependency="AIO.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

main.obj: ../main.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: C2000 Compiler'
	"c:/ti/ccsv6/tools/compiler/c2000_6.2.10/bin/cl2000" -v28 -ml -mt -O0 --include_path="c:/ti/ccsv6/tools/compiler/c2000_6.2.10/include" --include_path="C:/ti/controlSUITE/device_support/f2802x/v220/f2802x_common/lib" --include_path="C:/ti/controlSUITE/development_kits/C2000_LaunchPad" -g --define="_FLASH" --define=NDEBUG --define="_DEBUG" --define="LARGE_MODEL" --quiet --verbose_diagnostics --diag_warning=225 --gen_func_subsections=on --output_all_syms --cdebug_asm_data --preproc_with_compile --preproc_dependency="main.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

nokia5110.obj: ../nokia5110.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: C2000 Compiler'
	"c:/ti/ccsv6/tools/compiler/c2000_6.2.10/bin/cl2000" -v28 -ml -mt -O0 --include_path="c:/ti/ccsv6/tools/compiler/c2000_6.2.10/include" --include_path="C:/ti/controlSUITE/device_support/f2802x/v220/f2802x_common/lib" --include_path="C:/ti/controlSUITE/development_kits/C2000_LaunchPad" -g --define="_FLASH" --define=NDEBUG --define="_DEBUG" --define="LARGE_MODEL" --quiet --verbose_diagnostics --diag_warning=225 --gen_func_subsections=on --output_all_syms --cdebug_asm_data --preproc_with_compile --preproc_dependency="nokia5110.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '



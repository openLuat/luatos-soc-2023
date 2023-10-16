ENTRY(Reset_Handler)
MEMORY
{
  ASMB_AREA(rwx) : ORIGIN = (0x00000000), LENGTH = ((0x00010000)-(0x00000000))
  MSMB_AREA(rwx) : ORIGIN = (0x00400000), LENGTH = ((0x00540000)-(0x00400000))
  FLASH_AREA(rx) : ORIGIN = (0x0087e000), LENGTH = (0x2c1000)
  PSRAM_AREA(rx) : ORIGIN = (0x08000000), LENGTH = ((0x08200000)-(0x08000000))
}
SECTIONS
{
  . = (0x0087e000);
  .vector :
  {
    KEEP(*(.isr_vector))
  } >FLASH_AREA
  .text :
  {
    *(.glue_7)
    *(.glue_7t)
    *(.vfpll_veneer)
    *(.v4_bx)
    *(.init*)
    *(.fini*)
    *(.iplt)
    *(.igot.plt)
    *(.rel.iplt)
    EXCLUDE_FILE(*libc_nano.a:lib_a-memset.o) *libc_nano.a:(.text*)
    *(.sect_cache_text.*)
    KEEP(*(SORT_BY_NAME(.sect_commonly_used_text*)))
    KEEP(*(SORT_BY_NAME(.sect_commonly_used_ro*)))
    *(.*rodata*)
    *(EXCLUDE_FILE(*libc*.a:*memset.o) .text*)
  } >FLASH_AREA
  .preinit_fun_array :
  {
      . = ALIGN(4);
      __preinit_fun_array_start = .;
      KEEP (*(SORT(.preinit_fun_array.*)))
      KEEP (*(.preinit_fun_array*))
      __preinit_fun_array_end = .;
      . = ALIGN(4);
  } > FLASH_AREA
  .drv_init_fun_array :
  {
      . = ALIGN(4);
      __drv_init_fun_array_start = .;
      KEEP (*(SORT(.drv_init_fun_array.*)))
      KEEP (*(.drv_init_fun_array*))
      __drv_init_fun_array_end = .;
      . = ALIGN(4);
  } > FLASH_AREA
  .task_fun_array :
  {
      . = ALIGN(4);
      __task_fun_array_start = .;
      KEEP (*(SORT(.task_fun_array.*)))
      KEEP (*(.task_fun_array*))
      __task_fun_array_end = .;
      . = ALIGN(4);
  } > FLASH_AREA
  .load_ap_piram_uncomp_msmb : ALIGN(4)
  {
    . = ALIGN(4);
    Load$$LOAD_AP_PIRAM_UNCOMP_MSMB$$Base = LOADADDR(.load_ap_piram_uncomp_msmb);
    Image$$LOAD_AP_PIRAM_UNCOMP_MSMB$$Base = .;
   *(.sect_uncompress_platPMRamcode_text.*)
    . = ALIGN(4);
  } >MSMB_AREA AT>FLASH_AREA
  Image$$LOAD_AP_PIRAM_UNCOMP_MSMB$$Length = SIZEOF(.load_ap_piram_uncomp_msmb);
  .load_ap_firam_psram : ALIGN(4)
  {
    . = ALIGN(4);
    Load$$LOAD_AP_FIRAM_PSRAM$$Base = LOADADDR(.load_ap_firam_psram);
    Image$$LOAD_AP_FIRAM_PSRAM$$Base = .;
    *(.sect_platFPSRAMRamcode_text.*)
    . = ALIGN(4);
  } >PSRAM_AREA AT>FLASH_AREA
  Image$$LOAD_AP_FIRAM_PSRAM$$Length = SIZEOF(.load_ap_firam_psram);
  .load_psram_shared : ALIGN(4)
  {
    . = ALIGN(4);
    Load$$LOAD_PSRAM_SHARED$$Base = LOADADDR(.load_psram_shared);
    Image$$LOAD_PSRAM_SHARED$$Base = .;
    *(.sect_platFPSRAMRWData_data.*)
 *decoder.a: (.data*)
 *decoder.a: (.stack*)
 *(.pic30w)
    . = ALIGN(4);
  } >PSRAM_AREA AT>FLASH_AREA
  Image$$LOAD_PSRAM_SHARED$$Length = SIZEOF(.load_psram_shared);
  .load_psram_shared_zi (NOLOAD):
  {
    . = ALIGN(4);
    Image$$LOAD_PSRAM_SHARED$$ZI$$Base = .;
    *(.sect_platFPSRAMZIData_bss.*)
 *decoder.a: (.bss*)
    . = ALIGN(4);
    Image$$LOAD_PSRAM_SHARED$$ZI$$Limit = .;
  } >PSRAM_AREA
  PROVIDE(flashXIPLimit = LOADADDR(.load_bootcode));
  .load_bootcode (0x00000000) : ALIGN(4)
  {
    . = ALIGN(4);
    Load$$LOAD_BOOTCODE$$Base = LOADADDR(.load_bootcode);
    Image$$LOAD_BOOTCODE$$Base = .;
    KEEP(*(.mcuVector))
 *(.sect_ramBootCode_text.*)
    *(.sect_qspi_text.*)
    *(.sect_flash_text.*)
    *(.sect_flash_pre2text.*)
 *(.sect_flashlock_text.*)
    . = ALIGN(4);
  } >ASMB_AREA AT>FLASH_AREA
    Image$$LOAD_BOOTCODE$$Length = SIZEOF(.load_bootcode);
  .load_ap_piram_asmb : ALIGN(4)
  {
   . = ALIGN(4);
   Load$$LOAD_AP_PIRAM_ASMB$$Base = LOADADDR(.load_ap_piram_asmb);
   Image$$LOAD_AP_PIRAM_ASMB$$Base = .;
   *(.sect_psPARamcode_text.*)
   *(.sect_platPARamcode_text.*)
   *(.sect_platdelay_text.*)
   *(.text.memset)
   *(.memcpy.armv7m)
   *(.recordNodeRO)
   . = ALIGN(4);
  } >ASMB_AREA AT>FLASH_AREA
  Image$$LOAD_AP_PIRAM_ASMB$$Length = SIZEOF(.load_ap_piram_asmb);
  .load_ap_firam_asmb : ALIGN(4)
  {
   . = ALIGN(4);
   Load$$LOAD_AP_FIRAM_ASMB$$Base = LOADADDR(.load_ap_firam_asmb);
   Image$$LOAD_AP_FIRAM_ASMB$$Base = .;
   *(.sect_psFARamcode_text.*)
   *(.sect_platFARamcode_text.*)
   . = ALIGN(4);
  } >ASMB_AREA AT>FLASH_AREA
  Image$$LOAD_AP_FIRAM_ASMB$$Length = SIZEOF(.load_ap_firam_asmb);
  .load_ap_rwdata_asmb : ALIGN(4)
  {
   . = ALIGN(4);
   Load$$LOAD_AP_FDATA_ASMB$$RW$$Base = LOADADDR(.load_ap_rwdata_asmb);
   Image$$LOAD_AP_FDATA_ASMB$$RW$$Base = .;
   *(.sect_platFARWData_data.*)
   . = ALIGN(4);
  } >ASMB_AREA AT>FLASH_AREA
  Image$$LOAD_AP_FDATA_ASMB$$Length = SIZEOF(.load_ap_rwdata_asmb);
  .load_ps_rwdata_asmb : ALIGN(4)
  {
    Load$$LOAD_PS_FDATA_ASMB$$RW$$Base = LOADADDR(.load_ps_rwdata_asmb);
    Image$$LOAD_PS_FDATA_ASMB$$RW$$Base = .;
    *(.sect_psFARWData_data.*)
    . = ALIGN(4);
  } >ASMB_AREA AT>FLASH_AREA
  Image$$LOAD_PS_FDATA_ASMB$$RW$$Length = SIZEOF(.load_ps_rwdata_asmb);
  .load_apos : ALIGN(4)
  {
    . = ALIGN(4);
    Load$$LOAD_APOS$$Base = LOADADDR(.load_apos);
    Image$$LOAD_APOS$$Base = .;
    *(.sect_freertos_eventgroups_text.*)
    *(.sect_freertos_heap6_text.*)
    *(.sect_freertos_list_text.*)
    *(.sect_freertos_queue_text.*)
    *(.sect_freertos_tasks_text.*)
    *(.sect_freertos_timers_text.*)
    *(.sect_freertos_port_text.*)
    . = ALIGN(4);
  } >ASMB_AREA AT>FLASH_AREA
  Image$$LOAD_APOS$$Length = SIZEOF(.load_apos);
  .load_ap_zidata_asmb (NOLOAD):
  {
   . = ALIGN(4);
   Image$$LOAD_AP_FDATA_ASMB$$ZI$$Base = .;
   *(.sect_platFAZIData_bss.*)
   . = ALIGN(4);
   Image$$LOAD_AP_FDATA_ASMB$$ZI$$Limit = .;
   Image$$LOAD_PS_FDATA_ASMB$$ZI$$Base = .;
   *(.sect_psFAZIData_bss.*)
   . = ALIGN(4);
   Image$$LOAD_PS_FDATA_ASMB$$ZI$$Limit = .;
   *(.sect_platPANoInit_bss.*)
   *(.sect_psFANoInit_data.*)
   *(.exceptCheck)
   *(.decompress)
  } >ASMB_AREA
  .unload_cpaon (0x0000A800) (NOLOAD):
  {
    *(.sect_rawflash_bss.*)
  } >ASMB_AREA
  .load_rrcmem (0x0000B000) (NOLOAD):
  {
    *(.rrcMem)
  } >ASMB_AREA
  .load_flashmem (0x0000C000) (NOLOAD):
  {
    *(.apFlashMem)
  } >ASMB_AREA
  .load_ap_piram_msmb : ALIGN(4)
  {
    . = ALIGN(4);
    Load$$LOAD_AP_PIRAM_MSMB$$Base = LOADADDR(.load_ap_piram_msmb);
    Image$$LOAD_AP_PIRAM_MSMB$$Base = .;
    *(.sect_psPMRamcode_text.*)
    *(.sect_platPMRamcode_text.*)
    . = ALIGN(4);
  } >MSMB_AREA AT>FLASH_AREA
  Image$$LOAD_AP_PIRAM_MSMB$$Length = SIZEOF(.load_ap_piram_msmb);
  .load_ap_firam_msmb : ALIGN(4)
  {
    . = ALIGN(4);
    Load$$LOAD_AP_FIRAM_MSMB$$Base = LOADADDR(.load_ap_firam_msmb);
    Image$$LOAD_AP_FIRAM_MSMB$$Base = .;
    *(.sect_freertos_cmsisos2_text.*)
    *(.sect_freertos_tlsf_text.*)
    *(.sect_platFMRamcode_text.*)
    . = ALIGN(4);
  } >MSMB_AREA AT>FLASH_AREA
  Image$$LOAD_AP_FIRAM_MSMB$$Length = SIZEOF(.load_ap_firam_msmb);
  .load_ap_ps_firam_msmb : ALIGN(4)
  {
   . = ALIGN(4);
   Load$$LOAD_AP_PS_FIRAM_MSMB$$Base = LOADADDR(.load_ap_ps_firam_msmb);
   Image$$LOAD_AP_PS_FIRAM_MSMB$$Base = .;
   *(.sect_psFMRamcode_text.*)
   . = ALIGN(4);
  } >MSMB_AREA AT>FLASH_AREA
  Image$$LOAD_AP_PS_FIRAM_MSMB$$Length = SIZEOF(.load_ap_ps_firam_msmb);
  .load_dram_bsp : ALIGN(4)
  {
    . = ALIGN(4);
    Load$$LOAD_DRAM_BSP$$Base = LOADADDR(.load_dram_bsp);
    Image$$LOAD_DRAM_BSP$$Base = .;
    *(.sect_bsp_spi_data.*)
    *(.sect_flash_data.*)
    *(.sect_flash_rt_data.*)
    *(.sect_gpr_data.*)
    *(.sect_apmu_data.*)
    *(.sect_apmutiming_data.*)
    *(.sect_bsp_data.*)
    *(.sect_platconfig_data.*)
    *(.sect_system_data.*)
    *(.sect_unilog_data.*)
    *(.sect_pad_data.*)
    *(.sect_ic_data.*)
    *(.sect_ecmain_data.*)
    *(.sect_slpman_data.*)
    *(.sect_bsp_usart_data.*)
    *(.sect_bsp_lpusart_data.*)
    *(.sect_timer_data.*)
    *(.sect_dma_data.*)
    *(.sect_adc_data.*)
    *(.sect_wdt_data.*)
 *(.sect_uart_device_data.*)
 *(.sect_usb_device_data.*)
    *(.sect_clock_data.*)
    *(.sect_hal_adc_data.*)
    *(.sect_hal_adcproxy_data.*)
    *(.sect_hal_alarm_data.*)
    *(.sect_excep_dump_data.*)
    . = ALIGN(4);
  } >MSMB_AREA AT>FLASH_AREA
    Image$$LOAD_DRAM_BSP$$Length = SIZEOF(.load_dram_bsp);
  .load_dram_bsp_zi (NOLOAD):
  {
    . = ALIGN(4);
    Image$$LOAD_DRAM_BSP$$ZI$$Base = .;
    *(.sect_bsp_spi_bss.*)
    *(.sect_flash_bss.*)
    *(.sect_flash_rt_bss.*)
    *(.sect_gpr_bss.*)
    *(.sect_apmu_bss.*)
    *(.sect_apmutiming_bss.*)
    *(.sect_bsp_bss.*)
    *(.sect_platconfig_bss.*)
    *(.sect_system_bss.*)
    *(.sect_unilog_bss.*)
    *(.sect_pad_bss.*)
    *(.sect_ic_bss.*)
    *(.sect_ecmain_bss.*)
    *(.sect_slpman_bss.*)
    *(.sect_bsp_usart_bss.*)
    *(.sect_bsp_lpusart_bss.*)
    *(.sect_timer_bss.*)
    *(.sect_dma_bss.*)
    *(.sect_adc_bss.*)
    *(.sect_wdt_bss.*)
    *(.sect_uart_device_bss.*)
 *(.sect_usb_device_bss.*)
    *(.sect_clock_bss.*)
    *(.sect_adc_bss.*)
    *(.sect_hal_trim_bss.*)
    *(.sect_hal_adcproxy_bss.*)
    *(.sect_hal_alarm_bss.*)
    *(.sect_alarm_bss.*)
    *(.sect_excep_dump_bss.*)
    *(.recordNodeZI)
    . = ALIGN(4);
  Image$$LOAD_DRAM_BSP$$ZI$$Limit = .;
  } >MSMB_AREA
  .unload_slpmem (NOLOAD):
  {
    *(.sleepmem)
  } >MSMB_AREA
  .load_dram_shared : ALIGN(4)
  {
    . = ALIGN(4);
    Load$$LOAD_DRAM_SHARED$$Base = LOADADDR(.load_dram_shared);
    Image$$LOAD_DRAM_SHARED$$Base = .;
    *(.data*)
    . = ALIGN(4);
  } >MSMB_AREA AT>FLASH_AREA
  Image$$LOAD_DRAM_SHARED$$Length = SIZEOF(.load_dram_shared);
  .load_dram_shared_zi (NOLOAD):
  {
    . = ALIGN(4);
    Image$$LOAD_DRAM_SHARED$$ZI$$Base = .;
    *(.sect_*_bss* .bss*)
    *(COMMON)
    *(.sect_decompress_bss.*)
    . = ALIGN(4);
    *(.stack*)
    Image$$LOAD_DRAM_SHARED$$ZI$$Limit = .;
    *(.USB_NOINIT_DATA_BUF)
  } >MSMB_AREA
  PROVIDE(end_ap_data = . );
  PROVIDE(start_up_buffer = 0x498F00);
  .load_up_buffer start_up_buffer(NOLOAD):
  {
    *(.sect_catShareBuf_data.*)
    Image$$LOAD_UP_BUFFER$$Limit = .;
  } >MSMB_AREA
  PROVIDE(end_up_buffer = . );
  heap_size = start_up_buffer - end_ap_data;
  ASSERT(heap_size>=0x20000,"ap use too much ram, heap less than min_heap_size_threshold!")
  ASSERT(end_up_buffer<=(0x00500000),"ap use too much ram, exceed to MSMB_APMEM_END_ADDR")
  .load_xp_sharedinfo (0x0053E000) (NOLOAD):
  {
  *(.shareInfo)
  } >MSMB_AREA
  .load_dbg_area (0x0053EF00) (NOLOAD):
  {
  *(.resetFlag)
  } >MSMB_AREA
  .unload_xp_ipcmem (0x0053F000) (NOLOAD):
  {
  } >MSMB_AREA
}
GROUP(
    libgcc.a
    libc.a
    libm.a
 )

/*
 * Copyright: (c) 2020-2022, 2022 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	venus_98m.lds
 * Purpose:
 * History:
 */

/*ram: 0x2000,0000 ~ 0x2018,0000*/
MEMORY
{
	flash       : ORIGIN = 0xfe190000, LENGTH = 0x70000
	sysinfo_seg : ORIGIN = 0x20000000, LENGTH = 0x200
	ram         : ORIGIN = 0x20000200, LENGTH = 0x17fe00   /* ram 1.5MB*/
	sdram       : ORIGIN = 0x00000200, LENGTH = 0x7ffe00   /* sdram 8MB*/

	beta_seg(rw)         :  org = 0xD3040000, len = 0x10000 /*at>flash*/
	scrm_seg(rw)         :  org = 0xD3030000, len = 0x10000
	cau_istr_seg(rw)     :  org = 0xD3060000, len = 0x2000  /*at>flash*/
	sc_intl_map0_seg(rw) :  org = 0xD30B0000, len = 0x200
	sc_intl_map1_seg(rw) :  org = 0xD30C0000, len = 0x200
	sc_intl_idx0_seg(rw) :  org = 0xD30D0000, len = 0x400
	sc_intl_idx1_seg(rw) :  org = 0xD30E0000, len = 0x400
}

ENTRY(_reset)

SECTIONS
{
	.flash_rom :
	{
	 . = ALIGN(4);
	 *(.srom.text)
	 *(.srom.rodata)
	} >flash

	.sysinfo.literal : 
	{
		_sysinfo_literal_start = ABSOLUTE(.);
		*(.sysinfo.literal)
		_sysinfo_literal_end = ABSOLUTE(.);
	} > sysinfo_seg 

	.ramtext :
	{
	 . = ALIGN(4);
	 *(._reset)
	 *(.iram0.text)
	} >ram

	.ramdata :
	{
	 . = ALIGN(4);
	 *(.dram0.rodata)
	 *(.dram0.data)
	 *(.vectors)
	} >ram

	.rambss (NOLOAD) :
	{
	 . = ALIGN(4);
	 _dram0_bss_start = .;
	 *(.dram0.bss)
	 _dram0_bss_end   = .;
	} >ram

	.text :
	{
	 . = ALIGN(4);
	 _text_start = .;
  	 _text_lma_start = LOADADDR(.text);
	 *(.text)
	 _text_end = .;
	} >sdram

	.slowtext :
	{
	 . = ALIGN(4);
	 _slowtext_start = .;
	 *(.slowtext)
	 _slowtext_end = .;
	} >sdram

	.rodata :
	{
	 . = ALIGN(4);
	 *(.rodata*)
	} >sdram

	.data :
	{
	 . = ALIGN(4);
	 _data_start = .;
  	 _data_lma_start = LOADADDR(.data);
	 *(.data)
	 *(.symtab)
	 *(.strtab)
	 *(.shstrtab)
	 _data_end = .;
	} >sdram

	.bss :
	{
	 . = ALIGN(4);
	 _bss_start = .;
	 *(.bss)
	 *(COMMON)
	 _bss_end = .;
	} >sdram

	.stack :
	{
	 *(.stack)
	} >sdram

	_heap_start = .;
	_heap_end   = ORIGIN(sdram) + LENGTH(sdram);

	.beta.data :
	{
	 *(.beta.data)
	} >beta_seg AT > sdram 

	.scrm (NOLOAD) :
	{
	 *(.scrm.data)
	} >scrm_seg 

	.cau_istr.data :
	{
	 *(.cau_istr.data)
	} >cau_istr_seg AT > sdram

	.sc_intl_map0.data (NOLOAD) :
	{
	 *(.sc_intl_map0.data)
	} >sc_intl_map0_seg 

	.sc_intl_map1.data (NOLOAD) :
	{
	 *(.sc_intl_map1.data)
	} >sc_intl_map1_seg

	.sc_intl_idx0.data (NOLOAD) :
	{
	 *(.sc_intl_idx0.data)
	} >sc_intl_idx0_seg 

	.sc_intl_idx1.data (NOLOAD) :
	{
	 *(.sc_intl_idx1.data)
	} >sc_intl_idx1_seg 

	.debug  0 :  { *(.debug) }
	.line  0 :  { *(.line) }
	.debug_srcinfo  0 :  { *(.debug_srcinfo) }
	.debug_sfnames  0 :  { *(.debug_sfnames) }
	.debug_aranges  0 :  { *(.debug_aranges) }
	.debug_pubnames  0 :  { *(.debug_pubnames) }
	.debug_info  0 :  { *(.debug_info) }
	.debug_abbrev  0 :  { *(.debug_abbrev) }
	.debug_line  0 :  { *(.debug_line) }
	.debug_frame  0 :  { *(.debug_frame) }
	.debug_str  0 :  { *(.debug_str) }
	.debug_loc  0 :  { *(.debug_loc) }
	.debug_macinfo  0 :  { *(.debug_macinfo) }
	.debug_weaknames  0 :  { *(.debug_weaknames) }
	.debug_funcnames  0 :  { *(.debug_funcnames) }
	.debug_typenames  0 :  { *(.debug_typenames) }
	.debug_varnames  0 :  { *(.debug_varnames) }
	.debug_ranges 0 : { *(.debug_ranges ) }
	/DISCARD/ :
	{
	 *(*)
	}
}

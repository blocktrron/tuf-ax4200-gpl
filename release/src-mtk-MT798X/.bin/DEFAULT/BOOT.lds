SECTIONS
{
  .bl2 0x00000000 :
  {
    bl2_begin = .;
    bl2.img
  }

  .fip 0x00180000 :
  {
    fip_begin = .;
    fip.bin
  }
}

flash image verify(128MB flash):
 a. nand bad => check if any bad block exist
 b. ubi detach
 c. tftpboot 0x46000000 flash.bin
 d. nand erase 0 0x8000000
 e. nand write 0x46000000 0 0x8000000
 f. reset

flash image verify(256MB flash, need 512MB DDR size):
 a. nand bad => check if any bad block exist
 b. ubi detach
 c. tftpboot 0x46000000 flash.bin
 d. nand erase 0 0x10000000
 e. nand write 0x46000000 0 0x10000000
 f. reset

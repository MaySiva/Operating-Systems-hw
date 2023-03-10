#include "os.h"
#include "stdint.h"


/*Calculating the number of levels of the page table
4 Kb a frame 
64 bit PTEs
512 children (9 bits per level)
64 -12 -7 =45 . 45/9 =5
5-level page table
*/
void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn)
{
   uint64_t offset; //The offset of the pte in the page table 
   uint64_t *table_pointer = phys_to_virt((pt << 12)); //Pointer of the page table root

   if (ppn != NO_MAPPING)
   {
      int i = 0;
      while (i < 4) // Start the page walk
      {
         offset = (vpn >> (45 - (9 * (i + 1)))) & 0x1ff;
         if ((*(table_pointer + offset) & 1) == 0) // Means there is no frame (page) allocated - then allocate.
         {
            *(table_pointer + offset) = (alloc_page_frame() << 12) | 1; // |1 is for marking valid
         }
         table_pointer = phys_to_virt(((*(table_pointer + offset) >> 12) << 12));
         i++;
      }
      // i == 4 last level
      offset = vpn & 0x1ff;
      *(table_pointer + offset) = (ppn << 12) | 1; // |1 is for marking valid
      return;
   }
   else // ppn == NO_MAPPING
   {
      int i = 0;
      while (i < 4) // Start the page walk
      {
         offset = (vpn >> (45 - (9 * (i + 1)))) & 0x1ff;
         if ((*(table_pointer + offset) & 1) == 0)
         {
            return;
         }
         table_pointer = phys_to_virt(((*(table_pointer + offset) >> 12) << 12));
         i++;
      }
      offset = vpn & 0x1ff;
      *(table_pointer + offset) = 0; // Destroy the vpn mapping
   }
}

uint64_t page_table_query(uint64_t pt, uint64_t vpn)
{
   uint64_t offset;//The offset of the pte in the page table 
   uint64_t *table_pointer = phys_to_virt((pt << 12)); //Pointer of the page table root

   int i = 0;
   while (i < 5)
   {
      offset = (vpn >> (45 - (9 * (i + 1)))) & 0x1ff;
      if ((*(table_pointer + offset) & 1) == 0) // Valid bit is 0
      {
         return NO_MAPPING;
      }
      if (i != 4)
      {
         table_pointer = phys_to_virt(((*(table_pointer + offset) >> 12) << 12));
      }

      i++;
   }
   if ((*(table_pointer + offset) & 1) == 0) // Checing the valid bit of the last level
   {
      return NO_MAPPING;
   }

   return *(table_pointer + offset) >> 12;
}

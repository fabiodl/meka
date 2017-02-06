//-----------------------------------------------------------------------------
// MEKA - checksum.c
// Checksum - Code
//-----------------------------------------------------------------------------

#include "shared.h"
#include <zlib.h>
#include "db.h"
#include "vlfn.h"

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

static void     mekacrc(t_meka_crc *dst, const u8 *data, int data_size)
{
    int         i;
    long        Checksum_8 [8];

    if ((data_size > 0x2000) && (data_size & 0x1FFF) != 0)
        data_size -= (data_size % 8192);
    for (i = 0; i < 8; i ++)
        Checksum_8 [i] = 0;
    for (i = 0; i < data_size; i++)
    {
        int v = data[i];
        Checksum_8 [v & 7] ++;
        Checksum_8 [v >> 5] ++;
    }
    for (i = 0; i < 8; i ++)
        Checksum_8 [i] &= 0xFF;

    // This is totally stupid. Can't use a cast, or something?
    // I remember that at some point I had a problem and reverted
    // to this formula, but it looks stupid now.
    dst->v[0] = dst->v[1] = 0x00000000;
    for (i = 0; i < 8; i ++)
        dst->v[(i & 4) ? 0 : 1] |= Checksum_8 [7 - i] << ((i & 3) * 8);
}

//-----------------------------------------------------------------------------
// Checksum_Perform(const u8 *data, int data_size)
// Compute checksums for given set of ROM and update appropriate date
//-----------------------------------------------------------------------------
// FIXME: should take a media in parameter?
//-----------------------------------------------------------------------------

#define SDSC_TAG_ADDR 0x7FE0
#define SDSC_TAG "SDSC"
#define SDSC_TAG_LENGTH 4
#define SDSC_PROGRAM_RELEASE_NOTES_ADDR 0x7FEE
#define FAKECRC_STRING "fakecrc"
#define FAKECRC_STRING_LEN 7

bool hasFakeCRC(const u8 *data,int data_size,u32* crc_crc32,t_meka_crc*  crc_mekacrc){
  if (memcmp(data+(SDSC_TAG_ADDR%data_size),SDSC_TAG,SDSC_TAG_LENGTH)) return false;
  int addr=SDSC_PROGRAM_RELEASE_NOTES_ADDR%data_size;   
  addr=data[addr]+(data[addr+1]<<8);
  if (memcmp(data+addr,FAKECRC_STRING,FAKECRC_STRING_LEN)) return false;
  if (sscanf ( (const char*)data+addr+FAKECRC_STRING_LEN , "%08x %08X%08X", crc_crc32, &crc_mekacrc->v[0], &crc_mekacrc->v[1]) != 3){
    Msg(MSGT_DEBUG, "Error in reading fakecrc");
    return false;
  }
  Msg(MSGT_DEBUG, "This ROM is using fakecrc");
  return true;
}


void            Checksum_Perform(const u8 *data, int data_size)
{


  if (!    hasFakeCRC(data,data_size,&g_media_rom.crc32,&g_media_rom.mekacrc)){
  t_meka_crc  crc_mekacrc;

  
  // Compute and store MekaCRC
  mekacrc(&crc_mekacrc, data, data_size);
  g_media_rom.mekacrc.v[0] = crc_mekacrc.v[0];
  g_media_rom.mekacrc.v[1] = crc_mekacrc.v[1];
  
  // Compute and store CRC32
  g_media_rom.crc32 = crc32(0, data, data_size);
  }
    // Print out checksums (debugging)
    // Msg(MSGT_DEBUG, "MekaCRC -> %08X.%08X ; CRC -> %08x", g_media_rom.mekacrc.v[0], g_media_rom.mekacrc.v[1], g_media_rom.crc32);

    // Find DB entry
    DB.current_entry = DB_Entry_Find(g_media_rom.crc32, &g_media_rom.mekacrc);

    // Update VLFN
    {
		char media_path[FILENAME_LEN];
        StrPath_RemoveDirectory(media_path, g_env.Paths.MediaImageFile);
        if (DB.current_entry)
            VLFN_AddEntry(media_path, DB.current_entry);
        else
            VLFN_RemoveEntry(media_path);
    }
}

//-----------------------------------------------------------------------------


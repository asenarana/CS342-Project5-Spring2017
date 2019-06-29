#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <stdint.h>

#include <linux/msdos_fs.h>

#define SECTORSIZE 512   //bytes
#define BLOCKSIZE  4096  // bytes - do not change this value

char diskname[48];
int  disk_fd;
unsigned char volumesector[SECTORSIZE];
unsigned char rootblock[BLOCKSIZE];

int get_sector (unsigned char *buf, int snum)
{
	off_t offset;
	int n;
	offset = snum * SECTORSIZE;
	lseek (disk_fd, offset, SEEK_SET);
	n  = read (disk_fd, buf, SECTORSIZE);
	if (n == SECTORSIZE)
		return (0);
	else {
		printf ("sector number %d invalid or read error.\n", snum);
		exit (1);
	}
}


void print_sector (unsigned char *s)
{
	int i;

	for (i = 0; i < SECTORSIZE; ++i) {
		printf ("%02x ", (unsigned char) s[i]);
		if ((i+1) % 16 == 0)
			printf ("\n");
	}
	printf ("\n");
}

void name_ext(char *fullname, char *name, char *ext)
{
    strncpy(name, fullname, 8);
    strncpy(ext, fullname + 8, 3);
    name[8] = '\0';
    ext[3] = '\0';
    for(int i = 0; i < 8; i++)
    {
        if(fullname[i] == 0x20)
            name[i] = '\0';
    }
}

int
main(int argc, char *argv[])
{
    char command[15];
    char filename[15];
    char fullname[15];
    char name[9];
    char ext[4];
    unsigned char tempblock[SECTORSIZE];
    struct fat_boot_sector *volume;
    struct msdos_dir_entry *rootdir;
    struct msdos_dir_entry *tempEntry;
    unsigned int rootsector;
    uint32_t clusternum;
    uint32_t *container;
    int sectornum;
    int offset;

	if (argc < 4) {
		printf ("wrong usage\n");
		exit (1);
	}
	strcpy (diskname, argv[1]);
    disk_fd = open (diskname, O_RDWR);
	if (disk_fd < 0) {
		printf ("could not open the disk image\n");
		exit (1);
	}
	if(strcmp(argv[2], "-p") != 0)
	{
        printf ("wrong usage\n");
		exit (1);
	}
	strcpy(command, argv[3]);
	get_sector (volumesector, 0);
	volume = (struct fat_boot_sector *)volumesector;
	rootsector = volume->reserved + ((unsigned int)(volume->fats) * ((volume->fat32).length))
        + ((volume->fat32).root_cluster - 2) * (unsigned int)(volume->sec_per_clus);
    for(int i = 0; i < BLOCKSIZE / SECTORSIZE; i++)
    {
        get_sector(rootblock + SECTORSIZE * i, rootsector + i);
    }
    rootdir = (struct msdos_dir_entry *)rootblock;

    if(strcmp(command, "volumeinfo") == 0)
    {
        printf("Bytes per logical sector: %u\n", volume->sector_size[2]);
        printf("Sectors per cluster: %u\n", volume->sec_per_clus);
        printf("Number of reserved sectors: %u\n", volume->reserved);
        printf("Number of FATs: %u\n", volume->fats);
        printf("Sectors per FAT: %u\n", (volume->fat32).length);
        printf("First cluster in root directory: %u\n", (volume->fat32).root_cluster);
    }
    else if(strcmp(command, "rootdir") == 0)
    {
        for(int j = 0; (rootdir + j)->name[0] != 0x00; j++) {
            tempEntry = rootdir + j;
            name_ext((char *)tempEntry->name, name, ext);
            printf("[name: %s]\t[extension: %s]\t[size: %d(bytes)]\n", name, ext, tempEntry->size);
            }
    }
    else if(strcmp(command, "blocks") == 0)
    {
        if( argc < 5)
        {
            printf ("wrong usage\n");
            exit (1);
        }
        strcpy(filename, argv[4]);
        for(int k = 0; (rootdir + k)->name[0] != 0x00; k++)
        {
            tempEntry = rootdir + k;
            name_ext((char *)tempEntry->name, name, ext);
            strcpy(fullname, name);
            fullname[strlen(name)] = '.';
            strcpy(fullname + strlen(name) + 1, ext);
            if(strcasecmp(fullname, filename) == 0) {
                if(tempEntry->size == 0)
                    break;
                printf("%s\n", filename);
                clusternum = (uint32_t)((int)tempEntry->starthi * 256 + tempEntry->start);
                while(clusternum != EOF_FAT32)
                {
                    printf("%d\t:\t%x\n", clusternum, clusternum);
                    sectornum = volume->reserved + clusternum / (SECTORSIZE / sizeof(int32_t));
                    offset = clusternum % (SECTORSIZE / sizeof(int32_t));
                    get_sector(tempblock, sectornum);
                    container = (uint32_t *)tempblock;
                    clusternum = container[offset];
                    if(clusternum == EOF_FAT32)
                        break;
                }
                break;
            }
        }
    }
    else
    {
        printf ("wrong usage\n");
		exit (1);
    }
	close (disk_fd);

	return (0);
}

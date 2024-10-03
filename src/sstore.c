/**
 * @file sstore.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2024-09-08
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "sstore.h"

#include <stdio.h>

#define SSIZE 2048

sstore_entry_t sstore[SSIZE];

FILE *sstorefd = NULL;
const char *sstorefname = NULL;
int sstoreidx = 0;



int sstore_init(const char *fname) {
  sstorefname = fname;
  sstorefd = fopen(sstorefname, "wb");
  if (sstorefd == NULL) {
    perror(sstorefname);
    return -1;
  }
  sstore_str("", 0);  // Add the empty string
  return 0;
}



int sstore_open(const char *fname) {
  sstorefname = fname;
  sstorefd = fopen(fname, "rb");
  if (sstorefd == NULL) {
    perror(sstorefname);
    return -1;
  }
  return 0;
}



char *sstore_get(sstore_pos_t pos) {
  static char buf[1024];

  if (sstorefd == NULL) {
    return NULL;
  }
  if (fseek(sstorefd, pos, SEEK_SET) == -1) {
    perror(sstorefname);
    return NULL;
  }
  sstore_len_t len;
  if (fread(&len, sizeof(len), 1, sstorefd) != 1) {
    perror(sstorefname);
    return NULL;
  }
  if (len >= sizeof(buf)) {
    return NULL;
  }
  if (fread(buf, 1, len, sstorefd) != len) {
    perror(sstorefname);
    return NULL;
  }
  buf[len] = '\0';
  return buf;
}



void sstore_close() {
  if (sstorefd != NULL) {
    fseek(sstorefd, 0, SEEK_END);
    printf("sstoreidx: %d sstoresize: %ld\n", sstoreidx, ftell(sstorefd));
    fclose(sstorefd);
    sstorefd = NULL;
  }
}



sstore_pos_t sstore_str(const char *str, sstore_len_t length) {
  if (sstorefd == NULL) {
    return SSTORE_ERR;
  }

  if (sstoreidx >= SSIZE) {
    return SSTORE_ERR;
  }

  fseek(sstorefd, 0, SEEK_END);
  long p = ftell(sstorefd);    // NOLINT
  if (p == -1) {
    perror(sstorefname);
    return SSTORE_ERR;
  }
  if (p + length + sizeof(sstore_pos_t) >= 0xFFFF) {
    return SSTORE_ERR;
  }

  hash_t h = hash(str, length);
  for (int i = 0; i < sstoreidx; i++) {
    if (sstore[i].hash == h) {
      return sstore[i].pos;
    }
  }
  sstore[sstoreidx].hash = h;
  sstore[sstoreidx].pos = (sstore_pos_t)p;

  fwrite(&length, sizeof(length), 1, sstorefd);
  fwrite(str, 1, length, sstorefd);

  sstoreidx++;

  return (sstore_pos_t)p;
}

/* 
    Efficient CRC bruteforcer

    Copyright (C) 2013 Cosmin Gorgovan <cosmin@linux-geek.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <sys/stat.h> 
#include <sys/mman.h>
#include <fcntl.h>


uint8_t reversed[256];

uint8_t reverse8_slow(uint8_t in) {
  uint8_t reversed = 0;
  reversed |= (in & 0x80) >> 7;
  reversed |= (in & 0x40) >> 5;
  reversed |= (in & 0x20) >> 3;
  reversed |= (in & 0x10) >> 1;
  reversed |= (in & 0x08) << 1;
  reversed |= (in & 0x04) << 3;
  reversed |= (in & 0x02) << 5;
  reversed |= (in & 0x01) << 7;
  
  return reversed;
}

void init_reverse_lookup_table() {
  for (int i = 0; i < 256; i++) {
    reversed[i] = reverse8_slow(i);
  }
}

uint16_t reverse8(uint8_t value) {
  return reversed[value];
}

uint16_t reverse16(uint16_t value) {
    uint16_t reversed;
    
    reversed = reverse8(value >> 8);
    reversed |= reverse8(value & 0xFF) << 8;
    
    return reversed;
}

uint64_t find_min(uint64_t count, int64_t *numbers) {
  uint64_t min = UINT64_MAX;
  
  for (uint64_t i = 0; i < count; i++) {
    if (numbers[i] < min) {
      min = numbers[i];
    }
  }
  
  return min;
}

uint16_t crc16(const uint8_t *buf, size_t len, uint16_t pol, uint16_t remainder, bool reverse_in) {
  for (int i = 0; i < len; i++) {
    if (reverse_in) {
      remainder ^= (reverse8(buf[i]) << 8);
    } else {
      remainder ^= buf[i] << 8;
    }
    for (uint8_t bit = 8; bit > 0; --bit) {
      if (remainder & 0x8000) {
        remainder = (remainder << 1) ^ pol;
      } else {
        remainder = (remainder << 1);
      }
    } // for
  }
  
  return remainder;
}

bool find_xors_16(uint32_t pol, long int no_of_samples, uint8_t **samples, off_t *input_size, uint32_t *crcs, bool reverse_in, bool reverse_out) {
  uint32_t xor_in;
  uint32_t xor_out;
  uint32_t crc;
  bool same;
  int printed = 0;
  bool found = false;
  
  // A bit messy, but 0xFFFF and 0x0000 are the most likely xor_in values
  for (xor_in = 0x1FFFF; xor_in != 0xFFFF; xor_in++) {
    xor_in &= 0xFFFF;
  
    crc = crc16(samples[0], input_size[0], pol, xor_in, reverse_in);
    if (reverse_out) crc = reverse16(crc);
    xor_out = crc ^ crcs[0];
    same = true;
    
    for (long int s = 1; same && (s < no_of_samples); s++) {
      crc = crc16(samples[s], input_size[s], pol, xor_in, reverse_in);
      if (reverse_out) crc = reverse16(crc);
      if ((crc ^ xor_out) != crcs[s]) {
        same = false;
      }
    }
    
    if (same) {
      if (!found) found = true;
      if (printed >= 3) {
        printf("...\n");
        return true;
      } else {
        printf("Found a solution: poly 0x%x, xor_in: 0x%x, xor_out: 0x%x, reverse_in: %s, reverse_out: %s\n",
               pol, xor_in, xor_out, reverse_in ? "true" : "false", reverse_out? "true" : "false");
        printed++;
      }
    } // if same
  } // for xor_in
  
  return found;
}

void find_poly_16(long int no_of_samples, uint8_t **samples, off_t *input_size, uint32_t *crcs) {
  off_t diff, min_size;
  bool same;
  uint32_t diff_crc;
  uint32_t pol;
  uint32_t crc1, crc1_r;
  uint32_t crc2, crc2_r;
  bool found;
  int candidates = 0;

  if (no_of_samples < 2) return;

  // Minimise data size
  min_size = find_min(no_of_samples, input_size);
  same = true;
  
  for (diff = 0; same && (diff < min_size); diff++) {
    for (long int s = 1; s < no_of_samples; s++) {
      if (samples[s][diff] != samples[0][diff]) {
        same = false;
      }
    } // for s
  } // for diff
  
  diff--;
  
  for (int reverse_in = 0; reverse_in < 2; reverse_in++) {
    for (pol = 0; pol <= 0xffff; pol++) {
      crc2 = crc16(&samples[0][diff], input_size[0] - diff, pol, 0, reverse_in);
      crc2_r = reverse16(crc2);
      same = true;
      
      for (long int s = 1; same && (s < no_of_samples); s++) {
        crc1 = crc2;
        crc1_r = crc2_r;
        crc2 = crc16(&samples[s][diff], input_size[s] - diff, pol, 0, reverse_in);
        crc2_r = reverse16(crc2);
        diff_crc = crcs[s-1] ^ crcs[s];
        
        if (((crc1 ^ crc2) != diff_crc) && ((crc1_r ^ crc2_r) != diff_crc)) {
          same = false;
        }
      } // for s
      
      if (same) {
        found = find_xors_16(pol, no_of_samples, samples, input_size, crcs, reverse_in, (crc1_r ^ crc2_r) == diff_crc);
        if (found) candidates++;
      }
    } // pol
  } // reverse_in
  
  if (candidates > 1) {
    printf("Found multiple candidates. Try to add more sample files for more accurate results\n");
  }
  
  return;
}

void print_instructions() {
  fprintf(stderr, "Usage: crc_rev width file1 crc1 file2 crc2 [... fileN crcN]\n");
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
  long int bitwidth, no_of_files;
  uint8_t **input;
  off_t *input_size;
  uint32_t *crcs;
  int fd;
  char *fname;
  struct stat fs;

  if (argc < 6 || (argc % 2) == 1) {
    print_instructions();
  }
  
  bitwidth = strtol(argv[1], NULL, 0);
  switch(bitwidth) {
    case 16:
      break;
    default:
      fprintf(stderr, "Unsupported width\n");
      print_instructions();
  }
  
  no_of_files = (argc - 2) / 2;
  input = malloc(sizeof(uint8_t*) * no_of_files);
  if (input == NULL) {
    fprintf(stderr, "Failed to allocate memory\n");
  }
  crcs = malloc(sizeof(uint32_t) * no_of_files);
  if (crcs == NULL) {
    fprintf(stderr, "Failed to allocate memory\n");
  }
  input_size = malloc(sizeof(off_t) * no_of_files);
  if (input_size == NULL) {
    fprintf(stderr, "Failed to allocate memory\n");
  }
  
  for (int i = 0; i < no_of_files; i++) {
    fname = argv[2 + i *2];

    crcs[i] = strtol(argv[3 + i *2], NULL, 0);
    printf("CRC for file %s: 0x%x\n", fname, crcs[i]);
  
    fd = open(fname, O_RDONLY);
    if (fd == -1) {
      fprintf(stderr, "Failed to open file: %s\n", fname);
      exit(EXIT_FAILURE);
    }
    fstat(fd, &fs);
    
    input[i] = mmap(NULL, fs.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (input[i] == MAP_FAILED) {
      fprintf(stderr, "Failed to map file: %s\n", fname);
      exit(EXIT_FAILURE);
    }
    input_size[i] = fs.st_size;
  }
  
  init_reverse_lookup_table();

  find_poly_16(no_of_files, input, input_size, crcs);

  return EXIT_SUCCESS;
}


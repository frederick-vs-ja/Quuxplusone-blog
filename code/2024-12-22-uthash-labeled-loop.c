// Compile with -std=gnu99 or -std=c23

#include <stdio.h>
#include <string.h>
#include <uthash.h>

typedef struct Row {
  char *name;
  int columns[4];
  UT_hash_handle hh;
} Row;

Row *new_row(const char *name, int *columns) {
  Row *p = malloc(sizeof *p);
  p->name = strdup(name);
  memcpy(p->columns, columns, 4 * sizeof(columns));
  return p;
}

void free_row(Row *p) {
  free(p->name);
  free(p);
}

Row *new_table() { return NULL; }
void table_add_row(Row **table, const char *name, int *columns) {
  Row *row = new_row(name, columns);
  HASH_ADD_STR(*table, name, row);
}
void free_table(Row **table) {
  Row *row, *tmp;
  HASH_ITER(hh, *table, row, tmp) {
    HASH_DEL(*table, row);
    free_row(row);
  }
}

int main() {
  Row *table = new_table();
  Row *row, *tmp;

  table_add_row(&table, "0th", (int[4]){0,0,0,0});
  table_add_row(&table, "1st", (int[4]){0,0,0,0});
  table_add_row(&table, "2nd", (int[4]){0,1,0,0});
  table_add_row(&table, "3rd", (int[4]){0,0,0,0});

  HASH_ITER(hh, table, row, tmp) {
    for (int y = 0; y < 4; ++y) {
      printf("x=%s y=%d\n", row->name, y);
      if (row->columns[y] != 0) {
        goto BREAK_OUTER;
      }
    }
  }
  BREAK_OUTER:
  puts("Exited outer loop");

  free_table(&table);
}

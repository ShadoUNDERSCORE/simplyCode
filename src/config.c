// FILE *f = fopen("log", "a");
// fprintf(f, "HERE...");
// fclose(f);
// FILE *f = fopen("log", "a"); fprintf(f, "HERE..."); fclose(f);

#include "../include/config.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static char *trim(char *str);
static int str_to_bool(char *str);

SettingsBucket **config_load(void) {
  SettingsBucket **config = calloc(HASHMAP_SIZE, sizeof(SettingsBucket));
  config_init(config);

  FILE *config_stream = fopen(CONFIG_PATH, "r");
  if (!config_stream) return NULL;

  char *line = NULL;
  size_t line_size = 0;
  ssize_t line_len;
 
  while ((line_len = getline(&line, &line_size, config_stream)) != -1) {
    if (line_len > 1) {
      line[strcspn(line, "\n")] = '\0';
      char *key = trim(strtok(line, "="));
      char *value = trim(strtok(NULL, "="));
      Setting setting = {0};
      int get_success = hashmap_get_setting(config, key, &setting, NULL);
      if (get_success) {
        switch (setting.type) {
          case INT:
            Setting updated_setting_int = {.key = strdup(key), .type = INT, .val.i = atoi(value)};
            hashmap_update(config, &updated_setting_int);
            break;
          case BOOL:
            int bool_value = str_to_bool(value);
            if (bool_value > -1) {
              Setting updated_setting_bool = {.key = strdup(key), .type = BOOL, .val.b = bool_value};
              hashmap_update(config, &updated_setting_bool);
            }
            break;
          case STRING:
            Setting updated_setting_str = {.key = strdup(key), .type = STRING, .val.s = strdup(value)};
            hashmap_update(config, &updated_setting_str);
            break;
          default:
            break;
        }
      }
      // break;
    }
  }
  free(line);
  return config;
}

void config_init(SettingsBucket **config) {
  Setting defaluts[N_SETTINGS] = {
    {.key = "tab_size", .type = INT, .val.i = 4},
    {.key = "auto_indent", .type = BOOL, .val.b = true},
    {.key = "autoclose_brackets", .type = BOOL, .val.b = true},
    {.key = "line_nums_color", .type = INT, .val.i = 0x967218},
    {.key = "save", .type = STRING, .val.s = "CTRL-W"},
    {.key = "exit", .type = STRING, .val.s = "CTRL-Q"},
    {.key = "save_exit", .type = STRING, .val.s = "CTRL-S"}
  };
  for (int i = 0; i < N_SETTINGS; i++) {
    hashmap_update(config, &defaluts[i]);
  }
  return;
}

static char *trim(char *str) {
  if (str == NULL) return NULL;
    char* start = str;
    while (isspace((unsigned char)*start)) {
      start++;
    }
    if (*start == '\0') {
      *str = '\0';
      return str;
    }
    char* end = start;
    while (*end != '\0') {
      end++;
    }
    end--;
    while (end >= start && isspace((unsigned char)*end)) {
      end--;
    }
    end++;
    *end = '\0';
    return start;
}

static int str_to_bool(char *str) {
  if (!strcasecmp(str, "true")) {
    return 1;
  } else if (!strcasecmp(str, "false")) {
    return 0;
  } else {
    return -1;
  }
}

int hashmap_hash(char *key) {
  long index = 0;
  for (int i = 0, l = strlen(key); i < l; i++) {
    index += (int)key[i];
    if (1 & ((int)key[i] * l)) {
      index += (int)key[i] % l;
    } else {
      index *= (int)key[i] / l;
    }
  }
  index *= strlen(key);
  index ^= (1 << 2);
  return (int)index % (HASHMAP_SIZE - 1);
}

bool hashmap_get_setting(SettingsBucket **map, char *key, Setting *out, int *index_out) {
  int index = hashmap_hash(key);
  if (map[index] != NULL) {
    for (int i = 0; i < map[index]->n_items; i++) {
      // FILE *f = fopen("log", "a"); fprintf(f, "%s, %s\n", map[index]->settings[i].key, key); fclose(f);
      if (!strcasecmp(map[index]->settings[i].key, key)) {
        // FILE *f = fopen("log", "a"); fprintf(f, "MATCH!\n"); fclose(f);
        if (out) {
          *out = map[index]->settings[i];
        }
        if (index_out) {
          *index_out = i;
        }
        return true;
      }
    }
  }
  return false;
}

bool hashmap_get_value(SettingsBucket **map, char *key, ConfigValue *out) {
  Setting setting = {0};
  if (hashmap_get_setting(map, key, &setting, NULL)) {
    FILE *f = fopen("log", "a"); fprintf(f, "VPASS!\n"); fclose(f);
    *out = setting.val;
    return true;
  }
  return false;
}

void hashmap_update(SettingsBucket **map, Setting *updated_setting) {
  int index = hashmap_hash(updated_setting->key);
  if (map[index] != NULL) {
    int len = map[index]->n_items;
    if (map[index]->capacity < len - 2) {
      map[index]->settings = realloc(map[index]->settings,
                                     (map[index]->capacity + BUCKET_DEFAULT_SIZE) * sizeof(Setting));
      map[index]->capacity += BUCKET_DEFAULT_SIZE;
    }
    int bucket_index;
    if (hashmap_get_setting(map, updated_setting->key, NULL, &bucket_index)) {
      map[index]->settings[bucket_index] = *updated_setting;
      // memmove(&map[index]->settings[bucket_index], updated_setting, sizeof(Setting));
    } else {
      map[index]->settings[len] = *updated_setting;
      // memmove(&map[index]->settings[len], updated_setting, sizeof(Setting));
      map[index]->n_items++;
    }
  } else {
    SettingsBucket *new_bucket = malloc(sizeof(SettingsBucket));
    new_bucket->settings = calloc(BUCKET_DEFAULT_SIZE, sizeof(Setting));
    new_bucket->settings[0] = *updated_setting;
    // memmove(&new_bucket->settings[0], updated_setting, sizeof(Setting));
    new_bucket->n_items = 1;
    new_bucket->capacity = BUCKET_DEFAULT_SIZE;
    map[index] = new_bucket;
  }
  return;
}
// TODO: Free config

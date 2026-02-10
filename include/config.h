#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_PATH "config.ini"
#define HASHMAP_SIZE 50
#define BUCKET_DEFAULT_SIZE 8
#define N_SETTINGS 7

#include <stdbool.h>

typedef union {
  int i;
  bool b;
  char *s;
} ConfigValue;

typedef struct {
  char *key;
  enum {INT, BOOL, STRING} type;
  ConfigValue val;
} Setting;

typedef struct {
  int n_items;
  int capacity;
  Setting *settings;
} SettingsBucket;

SettingsBucket **config_load(void);
void config_init(SettingsBucket **config);
int hashmap_hash(char *key);
bool hashmap_get_setting(SettingsBucket **map, char *key, Setting *out, int *index_out);
bool hashmap_get_value(SettingsBucket **map, char *key, ConfigValue *out);
void hashmap_update(SettingsBucket **map, Setting updated_setting);

#endif

#include <stdlib.h>
#include <string.h>

#include "memory.hpp"
#include "object.hpp"
#include "table.hpp"
#include "value.hpp"
#include "vm.hpp"

// note: this was fixed in ch24

#define TABLE_MAX_LOAD 0.75

void initTable (Table* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void freeTable (Table* table) {
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable(table);
}

static Entry* findEntry (Entry* entries, int capacity, ObjString* key) {
    uint32_t index = key->hash % capacity;
    Entry* tombstone = NULL;
    
    while (1) {
        Entry* entry = &entries[index];
        if (entry->key == NULL)
        {
            // Empty entry
            if (IS_NIL(entry->value)) return tombstone != NULL ? tombstone : entry; 
            // Tombstone
            else { if (tombstone == NULL) tombstone = entry;  }
        }
        // We found our key!
        else if (entry->key == key) return entry; 

        index = (index + 1) % capacity;
    }
}


bool tableGet (Table* table, ObjString* key, Value* value) {
    if (table->count == 0) { return false; }

    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) { return false; }

    *value = entry->value;
    return true;
}

// adjust capacity is called when the table is full
static void adjustCapacity (Table* table, int capacity) {
    Entry* entries = ALLOCATE(Entry, capacity);

    // Clear newly allocated table
    for (int i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }

    // Re-insert old entries
    table->count = 0;
    for (int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i]; 
        if (entry->key == NULL) continue; 

        Entry* dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    // Free old table
    FREE_ARRAY(Entry, table->entries, table->capacity);

    table->entries = entries;
    table->capacity = capacity;
}

bool tableSet (Table* table, ObjString* key, Value value) {
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity= GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }

    Entry* entry = findEntry(table->entries, table->capacity, key);
    bool isNewKey = entry->key == NULL;
    if (isNewKey && IS_NIL(entry->value)) table->count++; 

    entry->key = key;
    entry->value = value;

    return isNewKey;
}

bool tableDelete (Table* table, ObjString* key)
{
    if (table->count == 0) return false; 

    // Find the entry
    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false; 

    // Insert a tombstone in place of the old pair
    entry->key = NULL;
    entry->value = BOOL_VAL(true);
    return true;
}

void tableAddAll (Table* from, Table* to) {
    for (int i = 0; i < from->capacity; i++) {
        Entry* entry = &from->entries[i];
        if (entry->key != NULL) tableSet(to, entry->key, entry->value); 
    }
}

ObjString* tableFindString (Table* table, const char* chars, int length, uint32_t hash) {
    // Empty table
    if (table->count == 0) return NULL; 

    uint32_t index = hash % table->capacity;
    while (1)
    {
        Entry* entry = &table->entries[index];
        // Is this entry empty & non-tombstone?
        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) return NULL; 
        }
        
        else if (entry->key->length == length && entry->key->hash == hash && memcmp(entry->key->chars, chars, length) == 0) return entry->key; 

        index = (index + 1) % table->capacity;
    }
}

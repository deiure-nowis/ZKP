#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "database.h"

#ifdef _WIN32
#include <direct.h>
#define MAKE_DIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#define MAKE_DIR(path) mkdir(path, 0777)
#endif

void db_init(QADatabase *db){
	db->sets = NULL;
	db->count = 0;
	db->capacity = 0;
}

void db_free(QADatabase *db){
	if(db->sets){
		for(size_t i = 0; i < db->count; i++){
			QASet *s = &db->sets[i];
			for(size_t j = 0; j < s->count; j++){
				free(s->pairs[j].question);
				free(s->pairs[j].answer);
				if(s->pairs[j].has_description)
					free(s->pairs[j].description);
			}
			free(s->pairs);
		}
		free(db->sets);
	}
	db_init(db);
}

QASet* db_add_set(QADatabase *db, const char *name){
	if(db->count >= db->capacity){
		size_t new_cap = (db->capacity == 0) ? 5 : db->capacity * 2;
		QASet *new_sets = realloc(db->sets, new_cap * sizeof(QASet));
		if(!new_sets) return NULL;
		db->sets = new_sets;
		db->capacity = new_cap;
	}
	QASet *s = &db->sets[db->count];
	strncpy(s->name, name, sizeof(s->name) - 1);
	s->name[sizeof(s->name) - 1] = '\0';
	s->pairs = NULL;
	s->count = 0;
	s->capacity = 0;
	db->count++;
	return s;
}

bool db_add_qa_to_set(QASet *set, const char *q, const char *a, const char *desc){
	if(set->count >= set->capacity){
		size_t new_cap = (set->capacity == 0) ? 10 : set->capacity * 2;
		QAPair *new_pairs = realloc(set->pairs, new_cap * sizeof(QAPair));
		if(!new_pairs) return false;
		set->pairs = new_pairs;
		set->capacity = new_cap;
	}
	QAPair *p = &set->pairs[set->count];
	p->id = set->count + 1;
	p->question = malloc(strlen(q) + 1);
	p->answer = malloc(strlen(a) + 1);
	
	if(!p->question || !p->answer) return false;
	strcpy(p->question, q);
	strcpy(p->answer, a);

	p->has_description = (desc != NULL && strlen(desc) > 0);
	if(p->has_description){
		p->description = malloc(strlen(desc) + 1);
		strcpy(p->description, desc);
	}else{
		p->description = NULL;
	}

	set->count++;
	return true;
}

void db_remove_set_at_index(QADatabase *db, size_t index) {
	if (index >= db->count) return;

	QASet *s = &db->sets[index];
	
	char filepath[256];
	snprintf(filepath, sizeof(filepath), "Data/%s.bin", s->name);
	remove(filepath); // Vymaže soubor, pokud existuje
	
	for (size_t j = 0; j < s->count; j++) {
		free(s->pairs[j].question);
		free(s->pairs[j].answer);
		if (s->pairs[j].has_description) free(s->pairs[j].description);
	}
	free(s->pairs);

	for (size_t i = index; i < db->count - 1; i++) {
		db->sets[i] = db->sets[i + 1];
	}
	db->count--;
}

bool db_save_all_sets(const QADatabase *db) {
	MAKE_DIR("Data"); // Vytvoří složku Data (ignoruje, pokud už existuje)

	for (size_t i = 0; i < db->count; i++) {
		QASet *s = &db->sets[i];
		char filepath[256];
		snprintf(filepath, sizeof(filepath), "Data/%s.bin", s->name);
		
		FILE *file = fopen(filepath, "wb");
		if (!file) continue;

		// Zapíšeme název sady a počet otázek
		fwrite(s->name, sizeof(char), sizeof(s->name), file);
		fwrite(&s->count, sizeof(size_t), 1, file);

		for (size_t j = 0; j < s->count; j++) {
			QAPair *p = &s->pairs[j];
			fwrite(&p->id, sizeof(unsigned int), 1, file);
			
			size_t q_len = strlen(p->question);
			fwrite(&q_len, sizeof(size_t), 1, file);
			fwrite(p->question, sizeof(char), q_len, file);
			
			size_t a_len = strlen(p->answer);
			fwrite(&a_len, sizeof(size_t), 1, file);
			fwrite(p->answer, sizeof(char), a_len, file);

			fwrite(&p->has_description, sizeof(bool), 1, file);
			if (p->has_description) {
				size_t d_len = strlen(p->description);
				fwrite(&d_len, sizeof(size_t), 1, file);
				fwrite(p->description, sizeof(char), d_len, file);
			}
		}
		fclose(file);
	}
	return true;
}

void db_load_all_sets(QADatabase *db) {
	DIR *dir = opendir("Data");
	if (!dir) return; // Složka ještě neexistuje, prostě se nic nenačte

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		size_t len = strlen(entry->d_name);
		// Hledáme pouze soubory končící na ".bin"
		if (len > 4 && strcmp(entry->d_name + len - 4, ".bin") == 0) {
			char filepath[512];
			snprintf(filepath, sizeof(filepath), "Data/%s", entry->d_name);
			
			FILE *file = fopen(filepath, "rb");
			if (!file) continue;
			
			char name[64];
			if (fread(name, sizeof(char), sizeof(name), file) == sizeof(name)) {
				QASet *s = db_add_set(db, name);
				
				size_t qa_count;
				if (fread(&qa_count, sizeof(size_t), 1, file) == 1) {
					for (size_t j = 0; j < qa_count; j++) {
						unsigned int id;
						size_t q_len, a_len;
						
						fread(&id, sizeof(unsigned int), 1, file);
						
						fread(&q_len, sizeof(size_t), 1, file);
						char *q = malloc(q_len + 1);
						fread(q, sizeof(char), q_len, file);
						q[q_len] = '\0';
						
						fread(&a_len, sizeof(size_t), 1, file);
						char *a = malloc(a_len + 1);
						fread(a, sizeof(char), a_len, file);
						a[a_len] = '\0';

						bool has_desc;
						fread(&has_desc, sizeof(bool), 1, file);
						char *desc = NULL;
						if (has_desc) {
							size_t d_len;
							fread(&d_len, sizeof(size_t), 1, file);
							desc = malloc(d_len + 1);
							fread(desc, sizeof(char), d_len, file);
							desc[d_len] = '\0';
						}
						
						db_add_qa_to_set(s, q, a, has_desc ? desc : "");
						
						free(q);
						free(a);
						if (has_desc) free(desc);
					}
				}
			}
			fclose(file);
		}
	}
	closedir(dir);
}

void db_remove_qa_at_index(QASet *set, size_t index){
	if(index >= set->count) return;
	
	free(set->pairs[index].question);
	free(set->pairs[index].answer);
	if(set->pairs[index].has_description)
		free(set->pairs[index].description);

	for(size_t i = index; i < set->count - 1; i++){
		set->pairs[i] = set->pairs[i + 1];
		set->pairs[i].id = i + 1;
	}
	
	set->count--;
}

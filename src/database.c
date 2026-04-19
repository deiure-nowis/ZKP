#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "database.h"

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

bool db_add_qa_to_set(QASet *set, const char *q, const char *a){
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
	
	set->count++;
	return true;
}

bool db_save_to_file(const QADatabase *db, const char *filename){
	FILE *file = fopen(filename, "wb");
	if(!file) return false;

	fwrite(&db->count, sizeof(size_t), 1, file);

	for(size_t i = 0; i < db->count; i++){
		QASet *s = &db->sets[i];
		fwrite(s->name, sizeof(char), sizeof(s->name), file);
		fwrite(&s->count, sizeof(size_t), 1, file);

		for(size_t j = 0; j < s->count; j++){
			QAPair *p = &s->pairs[j];
			fwrite(&p->id, sizeof(unsigned int), 1, file);
			
			size_t q_len = strlen(p->question);
			fwrite(&q_len, sizeof(size_t), 1, file);
			fwrite(p->question, sizeof(char), q_len, file);
			
			size_t a_len = strlen(p->answer);
			fwrite(&a_len, sizeof(size_t), 1, file);
			fwrite(p->answer, sizeof(char), a_len, file);
		}
	}
	fclose(file);
	return true;
}

bool db_load_from_file(QADatabase *db, const char *filename){
	FILE *file = fopen(filename, "rb");
	if(!file) return false;

	db_free(db);

	size_t set_count;
	if(fread(&set_count, sizeof(size_t), 1, file) != 1){
		fclose(file);
		return false;
	}

	for(size_t i = 0; i < set_count; i++){
		char name[64];
		size_t qa_count;
		
		fread(name, sizeof(char), sizeof(name), file);
		QASet *s = db_add_set(db, name);
		
		fread(&qa_count, sizeof(size_t), 1, file);
		
		for(size_t j = 0; j < qa_count; j++){
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
			
			db_add_qa_to_set(s, q, a);
			free(q);
			free(a);
		}
	}
	fclose(file);
	return true;
}

void db_remove_qa_at_index(QASet *set, size_t index){
	if(index >= set->count) return;
	
	// Uvolnění paměti mazaného prvku
	free(set->pairs[index].question);
	free(set->pairs[index].answer);
	
	// Posuneme zbytek pole doleva
	for(size_t i = index; i < set->count - 1; i++){
		set->pairs[i] = set->pairs[i + 1];
		set->pairs[i].id = i + 1; // Převrtání ID pro pořadí
	}
	
	set->count--;
}

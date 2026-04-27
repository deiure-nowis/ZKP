#ifndef DATABASE_H
#define DATABASE_H

#include <stddef.h>
#include <stdbool.h>

typedef struct{
	unsigned int id;
	char *question;
	char *answer;
	bool has_description;
	char *description;
} QAPair;

typedef struct{
	char name[64];
	QAPair *pairs;
	size_t count;
	size_t capacity;
} QASet;

typedef struct{
	QASet *sets;
	size_t count;
	size_t capacity;
} QADatabase;

void db_init(QADatabase *db);
void db_free(QADatabase *db);

void db_load_all_sets(QADatabase *db);
bool db_save_all_sets(const QADatabase *db);

QASet* db_add_set(QADatabase *db, const char *name);
bool db_add_qa_to_set(QASet *set, const char *q, const char *a, const char *desc);
void db_remove_qa_at_index(QASet *set, size_t index);
void db_remove_set_at_index(QADatabase *db, size_t index);

#endif // DATABASE_H

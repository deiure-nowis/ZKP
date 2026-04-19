#ifndef DATABASE_H
#define DATABASE_H

#include <stddef.h>
#include <stdbool.h>

typedef struct{
	unsigned int id;
	char *question;
	char *answer;
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

bool db_load_from_file(QADatabase *db, const char *filename);
bool db_save_to_file(const QADatabase *db, const char *filename);

QASet* db_add_set(QADatabase *db, const char *name);
bool db_add_qa_to_set(QASet *set, const char *q, const char *a);
void db_remove_qa_at_index(QASet *set, size_t index);

#endif // DATABASE_H

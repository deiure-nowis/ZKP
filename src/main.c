#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "database.h"
#include "ui.h"
#include "input.h"

#define DB_FILENAME "unidata.bin"

int main(){
	// Inicializace generátoru náhodných čísel pro náhodné zkoušení
	srand((unsigned int)time(NULL));

	QADatabase db;
	db_init(&db);
	db_load_from_file(&db, DB_FILENAME);

	bool running = true;
	while(running){
		clear_screen();
		printf("=== UNIVERZITNÍ Q&A DATABÁZE ===\n");
		printf("Počet sad: %zu\n\n", db.count);
		printf("[1] Správa sad (Přidat / Upravit / Smazat)\n");
		printf("[2] Cvičení (Zkoušení)\n");
		printf("[0] Uložit a ukončit\n");
		printf("==================================\n");
		
		int key = get_keypress();
		
		if(key == '1' || key == '+'){
			ui_manage_sets(&db);
		}else if(key == '2') {
			ui_practice_mode(&db);
		}else if(key == '0') {
			running = false;
		}
	}

	clear_screen();
	printf("Ukládám data...\n");
	db_save_to_file(&db, DB_FILENAME);
	db_free(&db);
	
	return 0;
}

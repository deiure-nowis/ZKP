#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ui.h"
#include "input.h"

// --- POMOCNÉ FUNKCE PRO ÚPRAVU ---

static void ui_edit_qa(QASet *set, size_t index){
	bool editing = true;
	while(editing){
		clear_screen();
		QAPair *p = &set->pairs[index];
		printf("=== ÚPRAVA OTÁZKY [ID: %u] ===\n", p->id);
		printf("Q: %s\n", p->question);
		printf("A: %s\n\n", p->answer);
		printf("[1] Upravit OTÁZKU\n");
		printf("[2] Upravit ODPOVĚĎ\n");
		printf("[X] SMAZAT záznam\n");
		printf("[0] Zpět\n");

		int key = get_keypress();
		if(key == '1'){
			char buf[1024];
			read_string("Nová otázka: ", buf, sizeof(buf));
			free(p->question);
			p->question = strdup(buf);
		}else if(key == '2'){
			char buf[2048];
			read_string("Nová odpověď: ", buf, sizeof(buf));
			free(p->answer);
			p->answer = strdup(buf);
		}else if(key == 'x' || key == 'X'){
			db_remove_qa_at_index(set, index);
			editing = false;
		}else if(key == '0' || key == KEY_ENTER){
			editing = false;
		}
	}
}

static void ui_browse_qa_in_set(QASet *set){
	int selected = 0;
	while(1){
		int total_options = set->count + 1; // Otázky + Zpět
		clear_screen();
		printf("=== PROHLÍŽENÍ SADY: %s ===\n", set->name);
		printf("(Enter = Upravit/Smazat, Šipky = Pohyb)\n\n");

		if(selected == 0) printf(" > [ ZPĚT ]\n");
		else printf("   [ ZPĚT ]\n");

		for(size_t i = 0; i < set->count; i++){
			if(selected == (int)i + 1)
				printf(" > Q: %s\n", set->pairs[i].question);
			else
				printf("   Q: %s\n", set->pairs[i].question);
		}

		int key = get_keypress();
		if(key == KEY_UP && selected > 0) selected--;
		if(key == KEY_DOWN && selected < total_options - 1) selected++;
		if(key == KEY_ENTER){
			if(selected == 0) break;
			ui_edit_qa(set, selected - 1);
			if(selected > (int)set->count) selected = set->count; // Pokud jsme smazali poslední
		}
	}
}

// --- HLAVNÍ EDITOR SADY ---

static void ui_set_editor(QASet *set){
	while(1){
		clear_screen();
		printf("=== EDITACE SADY: %s ===\n", set->name);
		printf("Počet otázek: %zu\n\n", set->count);
		printf("[1] Přidat nové otázky\n");
		printf("[2] Prohlížet / Upravit / Smazat otázky\n");
		printf("[0] Zpět\n");

		int key = get_keypress();
		if(key == '1'){
			while(1){
				printf("\n--- Přidávání (prázdná otázka = konec) ---\n");
				char q[1024], a[2048];
				read_string("Otázka: ", q, sizeof(q));
				if(strlen(q) == 0) break;
				read_string("Odpověď: ", a, sizeof(a));
				db_add_qa_to_set(set, q, a);
			}
		}else if(key == '2'){
			ui_browse_qa_in_set(set);
		}else if(key == '0'){
			break;
		}
	}
}

// --- VÝBĚR SADY (ZŮSTÁVÁ PODOBNÝ) ---

void ui_manage_sets(QADatabase *db){
	int selected = 0;
	while(1){
		int total_options = db->count + 2;
		clear_screen();
		printf("=== VOLBA SADY ===\n\n");
		if(selected == 0) printf(" > [ Zpět ]\n"); else printf("   [ Zpět ]\n");

		for(size_t i = 0; i < db->count; i++){
			if(selected == (int)i + 1) printf(" > %s (%zu)\n", db->sets[i].name, db->sets[i].count);
			else printf("   %s (%zu)\n", db->sets[i].name, db->sets[i].count);
		}

		int add_idx = total_options - 1;
		if(selected == add_idx) printf(" > [ + Nová sada ]\n"); else printf("   [ + Nová sada ]\n");

		int key = get_keypress();
		if(key == KEY_UP && selected > 0) selected--;
		if(key == KEY_DOWN && selected < add_idx) selected++;
		if(key == KEY_ENTER){
			if(selected == 0) break;
			if(selected == add_idx){
				char name[64];
				read_string("Název nové sady: ", name, sizeof(name));
				if(strlen(name) > 0){
					QASet *s = db_add_set(db, name);
					if(s) ui_set_editor(s);
				}
			}else{
				ui_set_editor(&db->sets[selected - 1]);
			}
		}
	}
}

// --- CVIČEBNÍ MÓD ---

// Pomocná funkce pro náhodné zamíchání indexů (Fisher-Yates shuffle)
static void shuffle_indices(size_t *array, size_t n){
	if(n > 1){
		for(size_t i = 0; i < n - 1; i++){
			size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
			size_t t = array[j];
			array[j] = array[i];
			array[i] = t;
		}
	}
}

// Vypsání všech otázek v sadě
static void ui_list_all_qa(QASet *set){
	clear_screen();
	printf("=== VÝPIS SADY: %s ===\n\n", set->name);
	if(set->count == 0){
		printf("Sada je prázdná.\n");
	}else{
		for(size_t i = 0; i < set->count; i++){
			printf("Q: %s\n", set->pairs[i].question);
			printf("A: %s\n", set->pairs[i].answer);
			printf("------------------------\n");
		}
	}
	printf("\nStiskněte libovolnou klávesu pro návrat...");
	get_keypress();
}

// Samotný cyklus zkoušení
static void ui_run_practice(QASet *set, bool random){
	if(set->count == 0){
		clear_screen();
		printf("Sada neobsahuje žádné otázky k procvičení!\n");
		get_keypress();
		return;
	}

	// Vytvoříme pole indexů, abychom mohli procházet náhodně bez úpravy původní databáze
	size_t *indices = malloc(set->count * sizeof(size_t));
	for(size_t i = 0; i < set->count; i++) indices[i] = i;

	if(random){
		shuffle_indices(indices, set->count);
	}

	for(size_t i = 0; i < set->count; i++){
		clear_screen();
		QAPair *p = &set->pairs[indices[i]];
		printf("=== CVIČENÍ: %s (%zu/%zu) ===\n", set->name, i + 1, set->count);
		printf("(Pro ukončení zkoušení a návrat napište '/q')\n\n");
		printf("Otázka: %s\n", p->question);

		char ans[2048];
		read_string("Vaše odpověď: ", ans, sizeof(ans));

		// Možnost kdykoliv odejít
		if(strcmp(ans, "/q") == 0){
			break; 
		}

		printf("\n");
		// Porovnání odpovědí (zatím case-sensitive přesný zápis)
		if(strcmp(ans, p->answer) == 0){
			printf("[V] Správně!\n");
		}else{
			printf("[X] Špatně!\n");
		}
		
		printf("Vaše odpověď:  %s\n", ans);
		printf("Uložená odp.:  %s\n\n", p->answer);
		
		printf("Stiskněte libovolnou klávesu pro další otázku...");
		get_keypress();
	}

	free(indices);
}

// Menu konkrétní sady ve cvičebním módu
static void ui_practice_set_menu(QASet *set){
	while(1){
		clear_screen();
		printf("=== CVIČENÍ SADY: %s ===\n", set->name);
		printf("Počet otázek v sadě: %zu\n\n", set->count);
		printf("[1] Vypsat všechny otázky a odpovědi\n");
		printf("[2] Projít popořadě\n");
		printf("[3] Projít náhodně\n");
		printf("[0] Zpět\n");

		int key = get_keypress();
		if(key == '1') ui_list_all_qa(set);
		else if(key == '2') ui_run_practice(set, false);
		else if(key == '3') ui_run_practice(set, true);
		else if(key == '0') break;
	}
}

// Výběr sady pro cvičení (podobné jako výběr pro úpravu, ale volá cvičení)
void ui_practice_mode(QADatabase *db){
	if(db->count == 0){
		clear_screen();
		printf("Nemáte uložené žádné sady!\nStiskněte libovolnou klávesu...");
		get_keypress();
		return;
	}

	int selected = 0;
	while(1){
		int total_options = db->count + 1; // [Zpět] + Sady
		clear_screen();
		printf("=== VÝBĚR SADY PRO CVIČENÍ ===\n\n");
		
		if(selected == 0) printf(" > [ Zpět ]\n"); else printf("   [ Zpět ]\n");

		for(size_t i = 0; i < db->count; i++){
			if(selected == (int)i + 1) printf(" > %s (%zu)\n", db->sets[i].name, db->sets[i].count);
			else printf("   %s (%zu)\n", db->sets[i].name, db->sets[i].count);
		}

		int key = get_keypress();
		if(key == KEY_UP && selected > 0) selected--;
		if(key == KEY_DOWN && selected < total_options - 1) selected++;
		if(key == KEY_ENTER){
			if(selected == 0) break;
			ui_practice_set_menu(&db->sets[selected - 1]);
		}
	}
}

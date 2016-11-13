#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* memset */
#include <unistd.h> /* close */
#include <stdbool.h>


/*
	-> Ciencias da Computacao
	->Sistemas Operacionais
	-> @Lucas Ranzi
	-> @Renan Carvalho

*/

// estrutura padrao para o fat
struct directory_entry{
	unsigned int dir;
	char name[20];
  unsigned int size_bytes;
  unsigned int start;
};

struct sector_data{
  unsigned char data[508];
  unsigned int next_sector;
};

struct root_table_directory{
  unsigned int free_blocks_list;
  struct directory_entry root_list_entry[127];
  unsigned char not_used[28];
};

struct table_directory{
  struct directory_entry dir_list_entry[16];
};

struct root_table_directory header;

//declaração de métodos
void buscar_table_directory(FILE * sys, char * path, struct table_directory * td );
void get_table_directory(FILE *sys, struct table_directory * td, unsigned int idx);
void save_position_list_td(FILE * sys,FILE * file, char * name_file,  int idx_empty,struct table_directory * td, int idxPos);
void save_data_sectores(FILE * sys,FILE * file, char * name_file, char * origem, char * destino,bool is_subdirectory);
void create_file_generic(FILE * sys,FILE * file, char * dir_name, char * origem, char * destino, bool is_subdirectory,	int idxPos);
void delete_value_pos_directory_table(FILE * sys, struct table_directory * td, char * dir_name, int idxPos);
unsigned int find_directory_header(char * dir_name);
unsigned int find_directory_table(struct table_directory * td, char * dir_name);
unsigned int find_file_table_directory(struct table_directory * td, char * dir_name);
int find_first_empty_dir_list_entry(struct table_directory * td);
bool checked_exist_directory_table(FILE * sys, struct table_directory * td, char * name);
char * get_last_name(char *name);
bool is_file(char * name);
struct directory_entry * get_entry_file_equals_name(int type, char * dir_name);


void save(FILE * sys, void * p, int size, int position){
	fseek(sys, (position*512), SEEK_SET);
	fwrite(p, size, 1, sys);
}

void erro_number_args(int argc, int numberArg){
	if(argc != (numberArg+1)){
		printf("Parametros incorretos foi passado %d ao inves de %d\n", argc-1, numberArg);
    exit(1);
	}
}

void buscar_table_directory_especifica(FILE * sys, char * path, struct table_directory * td , int idxParada){
	
	struct directory_entry * entry_file;
	char str[strlen(path)];
	strcpy(str, path);
	char * next = strtok(str, "/");
	char * before = next;
	next = strtok(NULL, "/");
	unsigned int idx_td;
	
	
	if(before){ //  existe mais diretorios ex: /teste/temp - entao pesquisa dentro do diretorio temp
		idxParada = idxParada - 1;
		
		//root
		entry_file = get_entry_file_equals_name(1,before);
		
		idx_td = find_directory_header(before);
		get_table_directory(sys, td, idx_td);

		before = next;
		next = strtok(NULL, "/");
			
		if(next != NULL){
			while (idxParada){
				idxParada = idxParada - 1;
				
				idx_td = find_directory_table(td, before);
				get_table_directory(sys, td, idx_td);
				
				//Retorna o último antes do arquivo 
   				if(!is_file(next)){
					before = next;
					next = strtok(NULL, "/");
				}
				else{
					return;
				}
			}
		}
	}
}

bool is_file(char * name){
		if(strchr(name, '.') != NULL ){
			return true;	
		}
		return false;
}

void buscar_table_directory(FILE * sys, char * path, struct table_directory * td ){

	//implementar aqui o mostrar
	struct directory_entry * entry_file;
	char str[strlen(path)];

	strcpy(str, path);

	char * next = strtok(str, "/");

	char * before = next;

	next = strtok(NULL, "/");

	unsigned int idx_td;


	if(before){ //  existe mais diretorios ex: /teste/temp - entao pesquisa dentro do diretorio temp
		//root
		//printf("BEFORE %s \n",before);
  		entry_file = get_entry_file_equals_name(1,before);
		
		idx_td = find_directory_header(before);
		get_table_directory(sys, td, idx_td);

		before = next;
		next = strtok(NULL, "/");
		
		if(next != NULL){
			while (before){
				
				idx_td = find_directory_table(td, before);
				get_table_directory(sys, td, idx_td);
				//Retorna o último antes do arquivo 
   				if(!is_file(next)){
					before = next;
					next = strtok(NULL, "/");
				}
				else{
					return;
				}
			}
		}
	}

}

//calculates file_size as bytes
int file_size(FILE* f){
	int curr = ftell(f);
	int size;
	fseek(f, 0L, SEEK_END);
	size = ftell(f);
	fseek(f, 0L, SEEK_SET);
	return size;
}

//verifica quantos sector_data sao necessarios
int number_sector(int size){
	return (size%508 == 0) ? size/508 : ((size/508) + 1);
}

struct sector_data get_free_sector(FILE *sys, unsigned int idx){
	struct sector_data sector;
	if(idx == 0) {
		printf("File systema full.\n");
		exit(1);
	}

	memset(&sector, 0, sizeof(struct sector_data));
	fseek(sys, (512*idx), SEEK_SET);
	fread(&sector, sizeof(struct sector_data), 1, sys);
	return sector;
}

char * get_last_name(char *name){
	
	char str[strlen(name)];
	strcpy(str, name);
	
	char *p;
	char * response = malloc(sizeof(char));
    p = strtok(str, "/");
	while (p != NULL) {
		response = p;
		p = strtok(NULL, "/");
		//printf("%s\n", p);
	}
		return response;
}


struct directory_entry * get_free_entry(){
	int i;
	for(i = 0; i < 127; i++){
		if(header.root_list_entry[i].start == 0) return &header.root_list_entry[i];
	}
	printf("Root list entry full. \n");
	exit(1);
}

struct directory_entry * get_entry_file_equals_name(int type, char * dir_name){
	int i;
	for(i = 0; i < 127; i++){
		//printf("%s , %d, %d fim -> \n", header.root_list_entry[i].name, header.root_list_entry[i].dir, header.root_list_entry[i].start );
		if(header.root_list_entry[i].dir == type ){
		/* LOG	
			printf("%d achou\n", type);
			printf("%d achou\n", header.root_list_entry[i].dir);
			printf("%s dir_name, %s root\n",dir_name, header.root_list_entry[i].name);
		*/	
			if(!strcmp(header.root_list_entry[i].name, dir_name)){
				//printf("%d achou\n", header.root_list_entry[i].start);
				return &header.root_list_entry[i];
			}
		}
	}
	printf("File or directory not found.\n");
	exit(1);
}

struct directory_entry * get_entry_dir_equals_name(char * dir_name){
	int i;
	for(i = 0; i < 127; i++){
		//printf("%s , %d, %d fim ->", header.root_list_entry[i].name, header.root_list_entry[i].dir, header.root_list_entry[i].start );
		if(header.root_list_entry[i].dir && !strcmp(header.root_list_entry[i].name, dir_name)){
			//printf("%s achou\n", header.root_list_entry[i].name);
			return &header.root_list_entry[i];
		}
	}
	printf("File or directory not found.\n");
	exit(1);
}

void delete_directory_table(FILE * sys, struct table_directory * td, char * dir_name, unsigned int idx_td){
	int i;
	for(i = 0; i < 16; i++){
		//printf("%s , %d, START %d, i=%d fim ->\n", td->dir_list_entry[i].name, td->dir_list_entry[i].dir, td->dir_list_entry[i].start,i);
		if(td->dir_list_entry[i].start != 0){
			printf("File or directory not empty.\n");
			exit(1);
		}
	}

	// cria um sector para zerar espaco dentro do sistema
	struct sector_data sector;
	memset(&sector, 0, sizeof(struct sector_data));
	sector.next_sector = header.free_blocks_list;

	header.free_blocks_list = idx_td;
	save(sys, &sector, sizeof(struct sector_data), idx_td);


	save(sys, &header, sizeof(struct root_table_directory), 0L);

}


void delete_value_pos_directory_table(FILE * sys, struct table_directory * td, char * dir_name, int idxPos){
	int i;
	for(i = 0; i < 16; i++){
		//printf("%s , %d, START %d, i=%d fim ->\n", td->dir_list_entry[i].name, td->dir_list_entry[i].dir, td->dir_list_entry[i].start,i);
		if(!strcmp( td->dir_list_entry[i].name, dir_name)){
			printf("Achou.\n");
			break;
		}
	}
	
	strcpy(td->dir_list_entry[i].name, "");
	td->dir_list_entry[i].size_bytes =0;
	td->dir_list_entry[i].dir = 0; //arquivo
	td->dir_list_entry[i].start = 0;
	
	save(sys, td, sizeof(struct table_directory), idxPos);
	
	/*
	struct table_directory * tdAUX = (struct table_directory *) malloc(sizeof(struct table_directory));
	fseek(sys, (512*(idxPos)), SEEK_SET);
	fread(tdAUX, sizeof(struct table_directory), 1, sys);
	
	for(i=0; i < 16; i++)
		printf("Name =  %s  SIZE= %d bytes  TYPE= %d  START=%d \n", tdAUX->dir_list_entry[i].name, tdAUX->dir_list_entry[i].size_bytes,  tdAUX->dir_list_entry[i].dir,tdAUX->dir_list_entry[i].start);
	*/
	

}



void delete_directory_header(FILE * sys, char * dir_name){

	struct directory_entry * entry = get_entry_dir_equals_name(dir_name);
	struct table_directory * td = (struct table_directory *) malloc(sizeof(struct table_directory));
	int i;

	//busca a table dentro do entry
	fseek(sys, (512*(entry->start)), SEEK_SET);
	fread(td, sizeof(struct table_directory), 1, sys);

	// nao consegue excluir pq tem coisas dentro
	for(i = 0; i < 16; i++){
		if(td->dir_list_entry[i].start != 0){
			printf("File or directory not empty.\n");
			exit(1);
		}
	}

	// cria um sector para zerar espaco dentro do sistema
	struct sector_data sector;
	memset(&sector, 0, sizeof(struct sector_data));
	sector.next_sector = header.free_blocks_list;

	header.free_blocks_list = entry->start;
	save(sys, &sector, sizeof(struct sector_data), entry->start);

	//limpa entry
	memset(entry, 0, sizeof(struct directory_entry));
	save(sys, &header, sizeof(struct root_table_directory), 0L);
}


bool checked_exist_directory_table(FILE * sys, struct table_directory * td, char * name){
	//printf("checked_exist_directory_table %s \n",name);
	int i;
	for(i = 0; i < 16; i++){
		if(strcmp(td->dir_list_entry[i].name, name) == 0){
			//printf("checked_exist_directory_table true  %s \n",td->dir_list_entry[i].name);
			return true;		
		}
	}
	//printf("checked_exist_directory_table false \n");
	return false;
}

int find_first_empty_dir_list_entry(struct table_directory * td){
	int i;
	for(i = 0; i < 16; i++){
		if(td->dir_list_entry[i].start == 0)
			return i;
	}
	
	return -1;
}

void create_directory_table(FILE * sys, struct table_directory * td, char * dir_name, unsigned int idx_td){
	int i;
	for(i = 0; i < 16; i++){
		if(!strcmp(td->dir_list_entry[i].name, dir_name)){
			printf("Cannot create directory ‘%s’: File exists.\n", dir_name);
			exit(1);
		}
	}
	for(i = 0; i < 16; i++){
		if(td->dir_list_entry[i].start == 0)
			 break;
	}
	struct sector_data sector;

	strcpy(td->dir_list_entry[i].name, dir_name);
	td->dir_list_entry[i].size_bytes = 0;
	td->dir_list_entry[i].dir = 1; //dir
	td->dir_list_entry[i].start = header.free_blocks_list;

	sector = get_free_sector(sys, header.free_blocks_list);
	header.free_blocks_list = sector.next_sector;

	//create new table_directory
	struct table_directory new_td;
	memset(&new_td, 0, sizeof(struct table_directory));
	save(sys, &new_td, sizeof(struct table_directory), td->dir_list_entry[i].start);
	save(sys, td, sizeof(struct table_directory), idx_td);
	save(sys, &header, sizeof(struct root_table_directory), 0L);
}

void create_directory_header(FILE *sys, char * dir_name){
	// validar se ja existe o nome
	int i;
	for(i = 0; i < 127; i++){
		if( !strcmp(header.root_list_entry[i].name, dir_name)){
			printf("Cannot create directory ‘%s’: Directory exists.\n", dir_name);
			exit(1);
		}
	}

	struct directory_entry * entry = get_free_entry();
	struct sector_data sector;

	strcpy((*entry).name, dir_name);
	(*entry).size_bytes = 0; 
	(*entry).dir = 1; //dir
	(*entry).start = header.free_blocks_list;

	sector = get_free_sector(sys, header.free_blocks_list);
	header.free_blocks_list = sector.next_sector;

	//create new table_directory
	struct table_directory td;
	memset(&td, 0, sizeof(struct table_directory));
	save(sys, &td, sizeof(struct table_directory), (*entry).start);
	save(sys, &header, sizeof(struct root_table_directory), 0L);
}

void get_table_directory(FILE *sys, struct table_directory * td, unsigned int idx){
	if(idx == 0) {
		printf("Systema de arquivos cheio! \n");
		exit(1);
	}
	memset(td, 0, sizeof(struct table_directory));
	fseek(sys, (512*idx), SEEK_SET);
	fread(td, sizeof(struct table_directory), 1, sys);
	//printf("%d origem, %d next_sector\n", freeBlock, sector.next_sector);
}




unsigned int find_directory_header(char * dir_name){
	int i;
	for(i = 0; i < 127; i++){
		//printf("name: %s, dir: %d, start: %d\n", header.root_list_entry[i].name, header.root_list_entry[i].dir, header.root_list_entry[i].start);
		if(header.root_list_entry[i].dir && !strcmp(header.root_list_entry[i].name, dir_name)){
			 return header.root_list_entry[i].start;
		}
	}
	printf("No such file or directory.\n");
	exit(1);
}

unsigned int find_file_table_directory(struct table_directory * td, char * dir_name){
	int i;
	for(i = 0; i < 16; i++){
	//	printf("TABLE -> name: %s, dir: %d, start: %d\n", td->dir_list_entry[i].name, td->dir_list_entry[i].dir, td->dir_list_entry[i].start);
		if(td->dir_list_entry[i].dir == 0 && !strcmp(td->dir_list_entry[i].name, dir_name)){
	//		printf("OK -> name: %s, dir: %d, start: %d\n", td->dir_list_entry[i].name, td->dir_list_entry[i].dir, td->dir_list_entry[i].start);
		
			return td->dir_list_entry[i].start;
		}
	}
	printf("No such file or directory.\n");
	exit(1);
}

unsigned int find_directory_table(struct table_directory * td, char * dir_name){
	int i;
	for(i = 0; i < 16; i++){
		//printf("TABLE -> name: %s, dir: %d, start: %d\n", td->dir_list_entry[i].name, td->dir_list_entry[i].dir, td->dir_list_entry[i].start);
		if(td->dir_list_entry[i].dir == 1 && !strcmp(td->dir_list_entry[i].name, dir_name)){
			//printf("OK -> name: %s, dir: %d, start: %d\n", td->dir_list_entry[i].name, td->dir_list_entry[i].dir, td->dir_list_entry[i].start);
		
			return td->dir_list_entry[i].start;
		}
	}
	printf("No such file or directory.\n");
	exit(1);
}

/* funcao para inicializar o sistema de arquivos*/
void init_system(int file_size){
  FILE *file;
  int i;
  //setar 0 na estrutura
  memset(&header, 0, sizeof(struct root_table_directory));
  header.free_blocks_list = 8;

  /* Escreve array em arquivo e fecha arquivo.*/
  if ((file = fopen("systemFat.bin", "w")) == NULL){
    perror("fopen ");
    exit(1);
  }
  fwrite(&header, sizeof(struct root_table_directory), 1, file);
	file_size = (file_size * 2000) - 8; // 1992 sector + 8 header = 2000 = 1MB
  
	for(i=1;i<file_size;i++){
    	struct sector_data sector;
    	memset(&sector, 0, sizeof(struct sector_data));
		if(i == file_size-1) {
			sector.next_sector = 0;
		}
    	else sector.next_sector = (8+i);
    	fwrite(&sector, sizeof(struct sector_data), 1, file);
  	}
	
  fclose(file);

	printf("Inicializado sistema com %d bytes\n", (file_size+8)*512);
}

void create_file(char *origem, char *destination){

		FILE *file, *sys;
		struct directory_entry *entry;
		struct sector_data sector;
		unsigned int i, position, count_directory = 0;
	
		//cria uma copia
		char str[strlen(destination)];
		strcpy(str, destination);
	
		char strAAA[strlen(destination)];
		strcpy(strAAA, destination);
	
		printf("Reading file to file system %s...\n", origem);
		//Read here
		if ((file = fopen(origem, "r+")) == NULL){
	    	printf("File not found!\n");
	    	exit(1);
	  	}	

		if ((sys = fopen("systemFat.bin", "r+")) == NULL){
			printf("File system not found!\n");
			exit(1);
		}
	
		// Buscar o header contendo todos os campos e valores
		fread(&header, sizeof(struct root_table_directory), 1, sys);
	
		//Quantidade de diretorios
		char * next = strtok(str, "/");
		char * before = next;
		next = strtok(NULL, "/");

	
		//Se não contains "." avança no subdiretorio
		if(next != NULL &&  !is_file(next)){
			while (next != NULL) {
					//printf("%s\n", next);
					count_directory = count_directory + 1;	
					before = next;
					next = strtok(NULL, "/");
			}
		}
	

		//recupera o nome do ultimo dado
		char *pAux,*last_name;
		last_name = before;
	
    	pAux = strtok(strAAA, "/");
		before= pAux;
				
		while (pAux != NULL) {
			if(!is_file(pAux)){
				before = pAux;
				pAux = strtok(NULL, "/");
			}else
				break;
		}
		
		//aloca espaço de memória para table_directory
	    struct table_directory * td = (struct table_directory *) malloc(sizeof(struct table_directory));

	    struct table_directory * tdAnt = (struct table_directory *) malloc(sizeof(struct table_directory));

	
		if(count_directory > 1){
			
			//retorna table_directory do último caminho
			buscar_table_directory(sys,destination,td);
				
			//retorna uma table lista anterior
			buscar_table_directory_especifica(sys,destination,tdAnt,count_directory - 1);
			
			//Busca a posição da ultima pasta
			int idxDirAnt = find_directory_table(tdAnt,before);
			
			//verifica se já existe no diretorio
			if(!checked_exist_directory_table(sys,td,last_name)){
			
				//Busca a primeira posição vazia da lista td
				int idx_empty_td = find_first_empty_dir_list_entry(td);
				
				//Salva info basica na lista
				save_position_list_td(sys,file,last_name,idx_empty_td,td,idxDirAnt);
		
				//Salva os dados do arquivo nos sectores
				save_data_sectores(sys,file,last_name,origem,destination,true);
					
			}else{
				printf("Arq. destino JAH existe no  diretorio");
				exit(0);
			}
			
		
		}else{
			save_data_sectores(sys,file,last_name,origem,destination,false);
		}
	
		fclose(file);
		fclose(sys);
		printf("Done - create\n" );
}

void save_data_sectores(FILE * sys,FILE * file, char * name_file, char * origem, char * destino, bool is_subdirectory){

	struct directory_entry *entry;
	struct sector_data sector;
	int size_bytes = file_size(file);
	int i,position;
	
	if(!is_subdirectory){
		entry = get_free_entry();	
		strcpy(entry->name, get_last_name(name_file));
		entry->size_bytes = size_bytes;
		entry->dir = 0; //arquivo
		entry->start = header.free_blocks_list;
			
	}
	int n_sector =number_sector(size_bytes);
	
	for(i = 0; i < n_sector; i++){
						
			//procura local vazio passando header.free_blocks_list
			sector = get_free_sector(sys, header.free_blocks_list);		
		
			//carrega dados do arquivo
			fseek(file, (508*i), SEEK_SET);
			fread(&sector.data, sizeof(unsigned char), 508, file);
		
		
			// salva posicao para gravar
			position = header.free_blocks_list;
		
			//passa o primeiro vazio
			header.free_blocks_list = sector.next_sector;
		
			//ultimo sector data
			if(i == (n_sector-1)) sector.next_sector = 0;

			save(sys, &sector, sizeof(struct sector_data), position);
	}
	
	save(sys, &header, sizeof(struct root_table_directory), 0L);
	
}
	
	
void save_position_list_td(FILE * sys,FILE * file, char * name_file,  int idx_empty,struct table_directory * td, int idxPos){
	/*LOG
	int i;
	for(i=0; i < 16; i++)
		printf("Name =  %s  SIZE= %d bytes  TYPE= %d  START=%d \n", td->dir_list_entry[i].name, td->dir_list_entry[i].size_bytes,  td->dir_list_entry[i].dir,td->dir_list_entry[i].start);
	*/
	
	
	strcpy(td->dir_list_entry[idx_empty].name, name_file);
	td->dir_list_entry[idx_empty].size_bytes = file_size(file);
	td->dir_list_entry[idx_empty].dir = 0; //arquivo
	td->dir_list_entry[idx_empty].start = header.free_blocks_list;

	/* LOG
	for(i=0; i < 16; i++)
		printf("Name =  %s  SIZE= %d bytes  TYPE= %d  START=%d \n", td->dir_list_entry[i].name, td->dir_list_entry[i].size_bytes,  td->dir_list_entry[i].dir,td->dir_list_entry[i].start);
	*/
	
	save(sys, td, sizeof(struct table_directory), idxPos);
	
	/* LOG
	struct table_directory * tdAUX = (struct table_directory *) malloc(sizeof(struct table_directory));
	fseek(sys, (512*(idxPos)), SEEK_SET);
	fread(tdAUX, sizeof(struct table_directory), 1, sys);
	
	for(i=0; i < 16; i++)
		printf("Name =  %s  SIZE= %d bytes  TYPE= %d  START=%d \n", tdAUX->dir_list_entry[i].name, tdAUX->dir_list_entry[i].size_bytes,  tdAUX->dir_list_entry[i].dir,tdAUX->dir_list_entry[i].start);
	*/
	
}

void read_file(char *origem, char *destination){
		
		FILE *file, *sys;
		int count_directory =0;

		printf("Read file %s \n", destination);

		if ((sys = fopen("systemFat.bin", "r+")) == NULL){
			printf("File system not found!\n");
			exit(1);
		}
	
	
		if ((file = fopen(origem, "w")) == NULL){
			perror("Erro ao criar arquivo - read ");
			exit(1);
		}

		// Buscar o header contendo todos os campos e valores
		fread(&header, sizeof(struct root_table_directory), 1, sys);
	
		//cria uma copia
		char str[strlen(destination)];
		strcpy(str, destination);

		char strAAA[strlen(destination)];
		strcpy(strAAA, destination);
	
		//Quantidade de diretorios
		char * dir_name = strtok(str, "/");
		char * before = dir_name;
		dir_name = strtok(NULL, "/");

	
		//Se não contains "." avança no subdiretorio
		if(dir_name != NULL &&  !is_file(dir_name)){
			while (dir_name != NULL) {
					count_directory = count_directory + 1;	
					before = dir_name;
					dir_name = strtok(NULL, "/");
			}
		}
		
	if(count_directory > 1){
		//aloca espaço de memória para table_directory
	    struct table_directory * td = (struct table_directory *) malloc(sizeof(struct table_directory));
	
		//retorna table_directory do último caminho
		buscar_table_directory(sys,destination,td);
		
		//verifica se existe no diretorio
		if(checked_exist_directory_table(sys,td,before)){
			
			//Recupera o stard dentro da lista directory__table
			int idxPos = find_file_table_directory(td, before);
			
			//Cria a arquivo
			create_file_generic(sys,file, before, origem, destination, true, idxPos);

		}	
	}else{
			create_file_generic(sys,file, before, origem, destination, false, 0)	;
	}
	
		fclose(file);
		fclose(sys);
		printf("Create file %s - read\n", origem );
}

void create_file_generic(FILE * sys,FILE * file, char * dir_name, char * origem, char * destino, bool is_subdirectory,	int idxPos){

	struct directory_entry * entry_file;
	struct sector_data sector;
	
	if(!is_subdirectory){
			// localizar arquivo na raiz
			entry_file = get_entry_file_equals_name(0,dir_name);
			idxPos = (*entry_file).start;
			printf("NOT is_subdirectory \n");
	}
	
		sector = get_free_sector(sys,idxPos );
		//printf("teste %d", sector.next_sector);
		while(sector.next_sector != 0){
			//grava no file na proxima
			//printf("teste \n" );
			fwrite(&sector.data, sizeof(char), 508, file);
			sector = get_free_sector(sys, sector.next_sector);
		}
			//grava no file na proxima
		fwrite(&sector.data, sizeof(char), 508, file);
}

void delete_file(char * name){
			FILE *sys;
		  struct directory_entry * entry_file;
			struct sector_data sector;

			//cria uma copia e pega nome
			char str[strlen(name)];
			strcpy(str, name);
			char * dir_name = strtok(str, "/");

			printf("Delete file %s \n", name);

			if ((sys = fopen("systemFat.bin", "r+")) == NULL){
				printf("File system not found!\n");
				exit(1);
			}

			// Buscar o header contendo todos os campos e valores
			fread(&header, sizeof(struct root_table_directory), 1, sys);
			// localizar arquivo na raiz
			entry_file = get_entry_file_equals_name(1,dir_name);
			unsigned int idx_sector_init = (*entry_file).start;
			//printf(" index sector = %d\n", idx_sector_init);
			memset(&(*entry_file), 0, sizeof(struct directory_entry));
			//printf(" index sector = %d\n",  (*entry_file).start);

			unsigned int temp = idx_sector_init;
			do {
				sector = get_free_sector(sys, temp);
				temp = sector.next_sector;
			} while(sector.next_sector != 0);

			sector.next_sector = header.free_blocks_list;
			//save
			fseek(sys, (temp*512), SEEK_SET);
			fwrite(&sector, sizeof(struct sector_data), 1, sys);

			header.free_blocks_list = idx_sector_init;

			//save header
			fseek(sys, 0L, SEEK_SET);
			fwrite(&header, sizeof(struct root_table_directory), 1, sys);

			fclose(sys);
			printf("Delete system file %s success.\n", name);
}

void create_dir(char * path){
		FILE *sys;
		bool have_file = false;

		if ((sys = fopen("systemFat.bin", "r+")) == NULL){
			printf("File not found!\n");
			exit(1);
		}

		// Buscar o header contendo todos os campos e valores
		fread(&header, sizeof(struct root_table_directory), 1, sys);
	

		char str[strlen(path)];
		strcpy(str, path);
		char * dir = strtok(str, "/");
	
		// validar se é diretorio 
		if(dir != NULL){
			if(is_file(dir)){
				printf("error command to delete file \n");
				exit(0);
			}
		}
	
		struct table_directory * td = (struct table_directory *) malloc(sizeof(struct table_directory));
		char * temp = dir;

		dir = strtok(NULL, "/");
	
			
		if(dir != NULL){
			if(is_file(dir)){
				printf("error command to delete file \n");
				exit(0);
			}			
		}	
		unsigned int idx_td;
		if(dir){ //  existe mais diretorios ex: /teste/temp - entao pesquisa dentro do diretorio temp
			
			idx_td = find_directory_header(temp);
			get_table_directory(sys, td, idx_td);

			temp = dir;
			dir = strtok(NULL, "/");

			while (dir){
			
				if(is_file(dir)){
					printf("error command to delete file \n");
					exit(0);
				}
			
				idx_td = find_directory_table(td, temp);
				get_table_directory(sys, td, idx_td);
			
				temp = dir;
	 			dir = strtok(NULL, "/");
			}

			create_directory_table(sys, td, temp, idx_td);

		}else{ // nao existe mais diretorios ex: /teste - entao cria o diretorio no header
			create_directory_header(sys, temp);
		}

		fclose(sys);
		printf("Create dir %s - success.\n", path);
}


void delete_dir(char * path){
		FILE *sys;

		if ((sys = fopen("systemFat.bin", "r+")) == NULL){
			printf("File not found!\n");
			exit(1);
		}

		// Buscar o header contendo todos os campos e valores
		fread(&header, sizeof(struct root_table_directory), 1, sys);
	
		// validar se é diretorio RENAN

		char str[strlen(path)];
		strcpy(str, path);
		char * next = strtok(str, "/");

		if(next != NULL && is_file(next)){
			printf("error command to delete file. \n");
			exit(0);
		}
		struct table_directory * tdAnt = (struct table_directory *) malloc(sizeof(struct table_directory));	
		struct table_directory * td = (struct table_directory *) malloc(sizeof(struct table_directory));
		char * before = next;
		next = strtok(NULL, "/");
		unsigned int idx_td,idx_td_ant;
		printf("top before %s next %s\n", before, next );
	
		if(next != NULL && is_file(next)){
			printf("error command to delete file. \n");
			exit(0);
		}
	
		if(next){ //  existe mais diretorios ex: /teste/temp - entao pesquisa dentro do diretorio temp
			idx_td = find_directory_header(before);
			get_table_directory(sys, tdAnt, idx_td);
			before = next;
			next = strtok(NULL, "/");
			
			while (next){

				if(next != NULL && is_file(next)){
					printf("error command to delete file. \n");
					exit(0);
				}
				
				idx_td = find_directory_table(tdAnt, before);
				get_table_directory(sys, tdAnt, idx_td);
				
				before = next;	
				next = strtok(NULL, "/");
			}
		
				idx_td_ant= idx_td;
			
				idx_td = find_directory_table(tdAnt, before);
				get_table_directory(sys, td, idx_td);
			
				delete_directory_table(sys, td, before, idx_td);
			
				delete_value_pos_directory_table(sys,tdAnt, before, idx_td_ant);

		}else{ // nao existe mais diretorios ex: /teste - entao cria o diretorio no header
			delete_directory_header(sys, before);
		}

		fclose(sys);
		printf("Delete dir %s - success.\n", path);
}

void show(char * path) {
	FILE *sys;
	int i;

	if ((sys = fopen("systemFat.bin", "r+")) == NULL){
		printf("File not found!\n");
		exit(1);
	}
	// Buscar o header contendo todos os campos e valores
	fread(&header, sizeof(struct root_table_directory), 1, sys);

	//implementar aqui o mostrar
	char str[strlen(path)];
	strcpy(str, path);
	char * next = strtok(str, "/");

	struct table_directory * td = (struct table_directory *) malloc(sizeof(struct table_directory));
	char * before = next;

	next = strtok(NULL, "/");
	unsigned int idx_td;

	if(before){ //  existe mais diretorios ex: /teste/temp - entao pesquisa dentro do diretorio temp
		idx_td = find_directory_header(before);
		get_table_directory(sys, td, idx_td);

		before = next;
		next = strtok(NULL, "/");

		while (before){
			idx_td = find_directory_table(td, before);
			get_table_directory(sys, td, idx_td);

			before = next;
			next = strtok(NULL, "/");
		}

		for(i = 0; i < 16; i++){
			if(td->dir_list_entry[i].start != 0){
				if(td->dir_list_entry[i].dir)
						printf("d %s\n", td->dir_list_entry[i].name);
				else
						printf("f %s %d bytes \n", td->dir_list_entry[i].name, td->dir_list_entry[i].size_bytes);
			}
		}

	}else{
			// quando for tratar o header
			for(i = 0; i < 127; i++){
				if(header.root_list_entry[i].start != 0) {
					if(header.root_list_entry[i].dir)
						printf("d %s\n", header.root_list_entry[i].name);
					else
						printf("f %s %d bytes \n", header.root_list_entry[i].name, header.root_list_entry[i].size_bytes);
				}
			}
		}

	fclose(sys);
}

int main(int argc,char *argv[]){

	if(argc < 1){
		printf("Parametros incorretos \n");
    exit(1);
	}

	if(!strcmp("-format", argv[1])){
		erro_number_args(argc, 2);
		int size = atoi(argv[2]);
	  	init_system(size);
	}else if(!strcmp("-create", argv[1])){
		erro_number_args(argc, 3);
		create_file(argv[2], argv[3]);
	}else if(!strcmp("-read", argv[1])){
		erro_number_args(argc, 3);
		read_file(argv[2], argv[3]);
	}else if(!strcmp("-del", argv[1])){
			erro_number_args(argc, 2);
		  delete_file(argv[2]);
	}else if(!strcmp("-mkdir", argv[1])){
			erro_number_args(argc, 2);
		  create_dir(argv[2]);
	}else if(!strcmp("-rmdir", argv[1])){
			erro_number_args(argc, 2);
		  delete_dir(argv[2]);
	}else if(!strcmp("-ls", argv[1])){
			erro_number_args(argc, 2);
		  show(argv[2]);
	}else{
			printf("Não encontrado\n" );
	}
}


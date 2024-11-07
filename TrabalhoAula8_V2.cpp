#include <stdio.h>
#include <stdlib.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_INSERE 14
#define MAX_BUSCA 7

struct busca
{
    char id_aluno[4];
    char sigla_disc[4];
} vet_b[MAX_BUSCA];

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_KEYS 3 // Número máximo de chaves para uma árvore-B de ordem 4
#define MAX_CHILD (MAX_KEYS + 1)
#define NIL -1
#define FILENAME "registros.bin"   // Arquivo de dados dos alunos
#define INDEX_FILENAME "index.bin" // Arquivo de índice

// Estrutura para representar o registro de um aluno
typedef struct
{
    char id[4];               // ID do aluno (3 caracteres + '\0')
    char discipline[4];       // Sigla da disciplina (3 caracteres + '\0')
    char name[50];            // Nome do aluno (até 50 caracteres)
    char discipline_name[50]; // Nome da disciplina (até 50 caracteres)
    float grade;              // Média do aluno
    float attendance;         // Frequência do aluno
} StudentRecord;
StudentRecord vet[MAX_INSERE];

// Estrutura para representar uma página da árvore-B, com endereço do registro
typedef struct
{
    int keycount;             // Número de chaves na página
    char keys[MAX_KEYS][7];   // Chaves ("ID+Disciplina")
    int children[MAX_CHILD];  // Pointers para filhos
    int record_rrn[MAX_KEYS]; // RRN do registro no arquivo de dados
} BTreePage;

// Estrutura de cabeçalho para o arquivo de índice
typedef struct
{
    int root_rrn;     // Endereço da raiz da árvore-B
    int insert_count; // Contador para número de entradas usadas para inserção
    int search_count; // Contador para número de entradas usadas para busca
} Header;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Função para inicializar o cabeçalho do arquivo de índice
void init_header(FILE *index_file)
{
    Header header;
    header.root_rrn = NIL;   // Inicialmente, a raiz é NIL (não existe)
    header.insert_count = 0; // Contador de inserções começa em 0
    header.search_count = 0; // Contador de buscas começa em 0

    fseek(index_file, 0, SEEK_SET);
    fwrite(&header, sizeof(Header), 1, index_file);
}

// Função para ler o cabeçalho do arquivo de índice
Header read_header(FILE *index_file)
{
    Header header;
    fseek(index_file, 0, SEEK_SET);
    fread(&header, sizeof(Header), 1, index_file);
    return header;
}

// Função para atualizar o cabeçalho do arquivo de índice
void update_header(FILE *index_file, Header *header)
{
    fseek(index_file, 0, SEEK_SET);
    fwrite(header, sizeof(Header), 1, index_file);
}

// Função para calcular o tamanho do registro
int calcularTamanhoRegistro(const StudentRecord &reg)
{
    int tamanho_nome_aluno = 0, tamanho_nome_disciplina = 0;
    for (int i = 0; i < 50; i++)
    {

        if (reg.name[i] == '\0')
        { // Verifica se encontrou o espaço vazio (0x00)
            break;
        }
        tamanho_nome_aluno++;
    }

    for (int i = 0; i < 50; i++)
    {
        if (reg.discipline_name[i] == '\0')
        { // Verifica se encontrou o espaço vazio (0x00)
            break;
        }
        tamanho_nome_disciplina++;
    }

    int tamanho = sizeof(reg.id) + 1 +
                  sizeof(reg.discipline) + 1 +
                  tamanho_nome_aluno * sizeof(char) + 1 + // +1 para o separador '#'
                  tamanho_nome_disciplina * sizeof(char) + 1 +
                  sizeof(reg.grade) + 1 +
                  sizeof(reg.attendance);
    return tamanho;
}

void initialize_btree(const char *index_filename)
{
    
    FILE *index_file = fopen(index_filename, "rb+");

    // Se o arquivo de índice não existe, cria e inicializa
    if (!index_file)
    {
        index_file = fopen(index_filename, "wb+");
        if (!index_file)
        {
            printf("Erro ao criar o arquivo de índice");
            exit(1);
        }

        // Inicializa o cabeçalho com raiz NIL e contadores zerados
        Header header;
        header.root_rrn = NIL;   // A árvore começa vazia, sem raiz
        header.insert_count = 0; // Contador de inserções zerado
        header.search_count = 0; // Contador de buscas zerado

        // Grava o cabeçalho no início do arquivo de índice
        fseek(index_file, 0, SEEK_SET);
        fwrite(&header, sizeof(header), 1, index_file);
        fflush(index_file);
        printf("Arquivo de índice inicializado com sucesso.\n");
    }
    else
    {
        printf("Arquivo de índice já existe e foi aberto para leitura/escrita.\n");
        Header header;
        header = read_header(index_file);
        printf("Root: %d\n", header.root_rrn);
        printf("Insert counter: %d\n", header.insert_count);
        printf("Search counter: %d\n", header.search_count);
    }

    // Fecha o arquivo para ser aberto posteriormente em modo "rb+" para operações de leitura/escrita
    fclose(index_file);
}

/////////////////////////////////////////////////////////////////////////////////

// Função para escrever um registro de aluno no arquivo
void write_student(FILE *file, StudentRecord *student)
{
    int size = calcularTamanhoRegistro(*student);
    fseek(file, 0, SEEK_END);
    fwrite(&size, sizeof(int), 1, file);
    // Escreve o conteúdo do registro
    fwrite(student->id, sizeof(student->id), 1, file);
    fputc('#', file);
    fwrite(student->discipline, sizeof(student->discipline), 1, file);
    fputc('#', file);

    // Insere nome_aluno caracter por caracter até encontrar 0x00
    for (int i = 0; i < 50; i++)
    {
        if (student->name[i] == '\0')
        {          // Verifica se encontrou o espaço vazio (0x00)
            break; // Pula para o próximo campo
        }
        fputc(student->name[i], file); // Escreve o caractere no arquivo
    }
    fputc('#', file);
    // Insere nome_disciplina caracter por caracter até encontrar 0x00
    for (int i = 0; i < 50; i++)
    {
        if (student->discipline_name[i] == '\0')
        {          // Verifica se encontrou o espaço vazio (0x00)
            break; // Pula para o próximo campo
        }
        fputc(student->discipline_name[i], file); // Escreve o caractere no arquivo
    }
    fputc('#', file);
    fwrite(&student->grade, sizeof(float), 1, file);
    fputc('#', file);
    fwrite(&student->attendance, sizeof(float), 1, file);
}

// Função para ler um registro de aluno a partir do arquivo
int read_student(FILE *file, StudentRecord *student)
{
    // Lê o tamanho do registro (número inteiro no início)
    int tamanhoRegistro = 0;
    fread(&tamanhoRegistro, sizeof(int), 1, file);

    // Lê o id_aluno e o delimitador '#'
    char id_aluno[4] = {0};
    fread(id_aluno, sizeof(char), 4, file);
    fgetc(file);

    id_aluno[3] = '\0';
    strcpy(student->id, id_aluno);

    // Lê a sigla_disciplina e o delimitador '#'
    char sigla_disciplina[4] = {0};
    fread(sigla_disciplina, sizeof(char), 4, file);
    fgetc(file);

    sigla_disciplina[3] = '\0';
    strcpy(student->discipline, sigla_disciplina);

    // Lê o nome_aluno até encontrar o delimitador '#'
    char nome_aluno[50] = {0};
    int i = 0;
    char c;
    while (fread(&c, sizeof(char), 1, file) == 1 && c != '#')
    {
        if (i < 50 - 1)
        {
            nome_aluno[i++] = c;
        }
    }
    nome_aluno[i] = '\0';
    strcpy(student->name, nome_aluno);

    // Lê o nome_disciplina até encontrar o delimitador '#'
    char nome_disciplina[50] = {0};
    i = 0;
    while (fread(&c, sizeof(char), 1, file) == 1 && c != '#')
    {
        if (i < 50 - 1)
        {
            nome_disciplina[i++] = c;
        }
    }
    nome_disciplina[i] = '\0';
    strcpy(student->discipline_name, nome_disciplina);

    // Lê a média e o delimitador '#'
    float media = 0.0;
    fread(&media, sizeof(float), 1, file);
    student->grade = media;
    fgetc(file);

    // Lê a frequência
    float frequencia = 0.0;
    fread(&frequencia, sizeof(float), 1, file);
    student->attendance = frequencia;
}

// Função para carregar o RRN da raiz da árvore-B
int get_root(FILE *index_file)
{
    int root;
    fseek(index_file, 0, SEEK_SET);
    fread(&root, sizeof(int), 1, index_file);
    return root;
}

// Função para definir o RRN da raiz da árvore-B
void set_root(int root)
{
    FILE *index_file = fopen(INDEX_FILENAME, "rb+");
    fseek(index_file, 0, SEEK_SET);
    fwrite(&root, sizeof(int), 1, index_file);
}

// Função para gravar uma página da árvore-B no arquivo de índice
void write_page(int rrn, BTreePage *page)
{
    FILE *index_file = fopen(INDEX_FILENAME, "rb+");
    if (!index_file)
    {
        perror("Erro ao abrir o arquivo de índice");
        exit(1);
    }
    fseek(index_file, sizeof(Header) + rrn * sizeof(BTreePage), SEEK_SET);
    fwrite(page, sizeof(BTreePage), 1, index_file);
    fclose(index_file);
}

// Função para ler uma página da árvore-B do arquivo de índice
void read_page(int rrn, BTreePage *page)
{
    FILE *index_file = fopen(INDEX_FILENAME, "rb");
    if (!index_file)
    {
        perror("Erro ao abrir o arquivo de índice");
        exit(1);
    }
    fseek(index_file, sizeof(Header) + rrn * sizeof(BTreePage), SEEK_SET);
    fread(page, sizeof(BTreePage), 1, index_file);
    fclose(index_file);
}

// Função para obter um novo RRN para uma página
int getpage()
{
    FILE *index_file = fopen(INDEX_FILENAME, "rb+");
    if (!index_file)
    {
        perror("Erro ao abrir o arquivo de índice");
        exit(1);
    }
    fseek(index_file, 0, SEEK_END);
    int rrn = (ftell(index_file) - sizeof(Header)) / sizeof(BTreePage);
    fclose(index_file);
    return rrn;
}

// Inicializa uma página da árvore-B
void init_page(BTreePage *page)
{
    page->keycount = 0;
    for (int i = 0; i < MAX_KEYS; i++)
    {
        memset(page->keys[i], '\0', sizeof(page->keys[i]));
        page->children[i] = NIL;
    }
    page->children[MAX_KEYS] = NIL;
}

// Função para criar uma nova raiz na árvore-B
int create_root(char *key, int left_child, int right_child)
{
    BTreePage new_root;
    init_page(&new_root);

    strcpy(new_root.keys[0], key);
    new_root.children[0] = left_child;
    new_root.children[1] = right_child;
    new_root.keycount = 1;

    int rrn = getpage();
    write_page(rrn, &new_root);
    set_root(rrn);
    return rrn;
}

///////////////////////////////////////////////////////////////////////////////////

// Insere uma chave em uma página da árvore-B
void insert_in_page(char *key, int record_rrn, int right_child, BTreePage *page)
{
    printf("Entrou no insert in page. \n");
    int j;
    for (j = page->keycount - 1; j >= 0 && strcmp(key, page->keys[j]) < 0; j--)
    {
        strcpy(page->keys[j + 1], page->keys[j]);
        page->children[j + 2] = page->children[j + 1];
        page->record_rrn[j + 1] = page->record_rrn[j];
    }
    printf("Passou do for de comparacoes do insert in page. \n");
    strcpy(page->keys[j + 1], key);
    page->record_rrn[j + 1] = record_rrn;
    page->children[j + 2] = right_child;
    page->keycount++;
}

// Busca uma chave em uma página específica
int search_node(char *key, BTreePage *page, int *pos)
{
    for (*pos = 0; *pos < page->keycount && strcmp(key, page->keys[*pos]) > 0; (*pos)++)
        ;
    return (*pos < page->keycount && strcmp(key, page->keys[*pos]) == 0);
}

// Função de busca na árvore-B
int search_in_tree(int rrn, char *key, int *page_rrn, int *pos, int *record_rrn)
{
    if (rrn == NIL)
        return 0;

    BTreePage page;
    read_page(rrn, &page);

    if (search_node(key, &page, pos))
    {
        *page_rrn = rrn;
        *record_rrn = page.record_rrn[*pos];
        return 1;
    }
    return search_in_tree(page.children[*pos], key, page_rrn, pos, record_rrn);
}

// Função para encontrar um aluno específico no arquivo de dados
int find_student(FILE *data_file, char *key, StudentRecord *student)
{
    rewind(data_file);
    char student_key[7];

    while (read_student(data_file, student))
    {
        sprintf(student_key, "%s%s", student->id, student->discipline);
        if (strcmp(student_key, key) == 0)
        {
            return 1;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void list_all_students(FILE *data_file, int rrn)
{
    if (rrn == NIL)
        return;

    BTreePage page;
    read_page(rrn, &page);

    for (int i = 0; i < page.keycount; i++)
    {
        list_all_students(data_file, page.children[i]);

        StudentRecord student;
        if (find_student(data_file, page.keys[i], &student))
        {
            printf("ID: %s, Disciplina: %s, Nome: %s, Média: %.2f, Frequência: %.2f\n",
                   student.id, student.discipline, student.name, student.grade, student.attendance);
        }
    }
    list_all_students(data_file, page.children[page.keycount]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void search_student(FILE *data_file, FILE *index_file, char *key)
{
    Header header = read_header(index_file);

    int root = header.root_rrn;
    int page_rrn, pos, record_rrn;

    if (search_in_tree(root, key, &page_rrn, &pos, &record_rrn))
    {
        printf("Chave %s encontrada, página %d, posição %d\n", key, page_rrn, pos);

        fseek(data_file, record_rrn * sizeof(StudentRecord), SEEK_SET);
        StudentRecord student;
        fread(&student, sizeof(StudentRecord), 1, data_file);

        printf("ID: %s, Disciplina: %s, Nome: %s, Média: %.2f, Frequência: %.2f\n",
               student.id, student.discipline, student.name, student.grade, student.attendance);
    }
    else
    {
        printf("Chave %s não encontrada\n", key);
    }

    header.search_count++;              // Atualiza o contador de buscas
    update_header(index_file, &header); // Grava o cabeçalho atualizado no arquivo
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void split(char *key, int r_child, BTreePage *p_oldpage, char *promo_key, int *promo_r_child, BTreePage *p_newpage)
{
    int j;
    int mid = 2; // Ponto médio para dividir
    char workkeys[MAX_KEYS + 1][7];
    int workchil[MAX_KEYS + 2];
    int work_rrns[MAX_KEYS + 1];

    printf("key = %s\n", key);

    // Copia chaves e filhos para arrays temporários
    for (j = 0; j < MAX_KEYS; j++)
    {
        strcpy(workkeys[j], p_oldpage->keys[j]);
        workchil[j] = p_oldpage->children[j];
        work_rrns[j] = p_oldpage->record_rrn[j];
    }
    workchil[MAX_KEYS] = p_oldpage->children[MAX_KEYS];

    // Insere a nova chave e o filho no local apropriado
    for (j = MAX_KEYS; j > 0 && strcmp(key, workkeys[j - 1]) < 0; j--)
    {
        strcpy(workkeys[j], workkeys[j - 1]);
        workchil[j + 1] = workchil[j];
        work_rrns[j] = work_rrns[j - 1];
    }
    
    strcpy(workkeys[j], key);
    workchil[j + 1] = r_child;
    work_rrns[j] = r_child;

    // Inicializa nova página e define a chave promovida
    *promo_r_child = getpage();
    init_page(p_newpage);
    strcpy(promo_key, workkeys[2]);
    printf("Chave %s promovida\n", promo_key);

    // Distribui chaves entre a página antiga e a nova
   char vazio[7] = "@@@@@@";
   for (j = 0; j < MAX_KEYS + 1; j++)
    {
        if(j<mid){
            strcpy(p_oldpage->keys[j], workkeys[j]);
            p_oldpage->record_rrn[j] = work_rrns[j];
        }else if(j==mid){
            p_oldpage->children[j] = workchil[j];
        }else{
            strcpy(p_newpage->keys[j-mid-1], workkeys[j]);
            p_newpage->children[j-mid-1] = workchil[j];
            p_newpage->record_rrn[j-mid-1] = work_rrns[j];
            
            strcpy(p_oldpage->keys[j - 1], vazio);
            p_oldpage->children[j] = NIL;
            p_oldpage->record_rrn[j - 1] = NIL;
        }       
    }
    p_oldpage->children[mid] = workchil[mid];
    p_newpage->children[j - mid - 1] = workchil[MAX_KEYS + 1];

    p_newpage->keycount = 1;
    p_oldpage->keycount = 2;
}

int insert_in_tree(int rrn, char *key, int record_rrn, int *promo_child, char *promo_key, int *promo_rrn)
{
    //BTreePage page;

    BTreePage page, newpage;
    
    int p_b_rrn;
    char p_b_key[7];

    if (rrn == NIL)
    {
        // Caso base: se o nó é NIL, a chave deve ser promovida ao nível superior
        strcpy(promo_key, key);
        *promo_rrn = record_rrn;
        *promo_child = NIL;
        return 1; // Indica que a promoção ocorreu
    }

    //printf("Passou 1\n");

    read_page(rrn, &page);

    //printf("Passou read_page. \n");

    int pos;
    int found = search_node(key, &page, &pos);

    if (found)
    {
        printf("Chave %s duplicada\n", key);
        return -1; // Indica falha na inserção por chave duplicada
    }

    //printf("Vai entrar prox insert tree. \n");

    // Chamada recursiva para inserir no filho apropriado
    int promoted = insert_in_tree(page.children[pos], key, record_rrn, promo_child, promo_key, promo_rrn);

    //printf("Passou o insert tree. \n");

    if (promoted == -1)
        return -1; // Retorna imediatamente se houve uma duplicação de chave
    if (promoted == 0)
        return 0; // Caso não ocorra promoção, termina a função

    //printf("Passou dos verificadores de promocao e duplicata. \n");

    // Se há espaço na página, insere a chave promovida na posição correta
    if (page.keycount < MAX_KEYS)
    {
        //printf("Entrou em page.keycount < MAX_KEYS .\n");
        insert_in_page(promo_key, *promo_rrn, *promo_child, &page);
        write_page(rrn, &page);
        return 0; // Não ocorre promoção adicional
    }
    else
    {
        // Divisão do nó: ocorre promoção de uma chave para o nível superior
        printf("Divisao de no\n");
        //split(promo_key, *promo_rrn, *promo_child, &page, promo_key, promo_rrn, promo_child);
        split(key, p_b_rrn, &page, promo_key, promo_child, &newpage);

        write_page(rrn, &page);
        return 1; // Indica que uma promoção adicional ocorreu
    }
}

void insert_student(FILE *index_file, FILE *data_file, StudentRecord *student)
{
    Header header = read_header(index_file);

    char key[7];
    sprintf(key, "%s%s", student->id, student->discipline);

    int promo_child;
    char promo_key[7];
    int promo_rrn;

    int root = header.root_rrn;

    // Primeiro, tentamos inserir na árvore-B
    int promoted = insert_in_tree(root, key, -1, &promo_child, promo_key, &promo_rrn);

    // Se a chave é duplicada, atualiza o contador e retorna
    if (promoted == -1)
    {
        // printf("Chave %s duplicada\n", key);
        header.insert_count++; // Atualiza o contador de inserções, mesmo para chaves duplicadas
        update_header(index_file, &header);
        return; // Termina a função
    }

    // Se a chave não for duplicada, insere o registro no arquivo de dados
    fseek(data_file, 0, SEEK_END);
    int record_rrn = ftell(data_file) /*/ sizeof(StudentRecord)*/;
    write_student(data_file, student);

    // Atualiza `promo_rrn` com o endereço do registro no arquivo de dados
    promo_rrn = record_rrn;

    // Atualiza a árvore com o RRN correto do registro
    if (promoted == 1)
    {
        // Caso a promoção ocorra na raiz, cria uma nova raiz
        root = create_root(promo_key, promo_rrn, promo_child);
        header.root_rrn = root; // Atualiza o RRN da raiz no cabeçalho
    }

    printf("Chave %s inserida com sucesso\n", key);
    header.insert_count++;              // Atualiza o contador de inserções para novas inserções
    update_header(index_file, &header); // Grava o cabeçalho atualizado no arquivo
}

/////////////////////////////////////////////////////////////////////////////////////////////

int main()
{
    FILE *index_file, *data_file, *file;

    // Abre o arquivo binário para leitura
    file = fopen("insere.bin", "rb");
    if (file == NULL)
    {
        printf("Nao foi possivel abrir o arquivo.\n");
        return 1;
    }

    // Le os registros do arquivo e armazena no vetor
    fread(vet, sizeof(vet), 1, file);

    // Fecha o arquivo
    fclose(file);

    // Exibe os registros lidos para verificar se tudo foi lido corretamente
    for (int i = 0; i < MAX_INSERE; i++)
    {
        printf("ID Aluno: %s\n", vet[i].id);
        printf("Sigla Disciplina: %s\n", vet[i].discipline);
        printf("Nome Aluno: %s\n", vet[i].name);
        printf("Nome Disciplina: %s\n", vet[i].discipline_name);
        printf("Media: %.2f\n", vet[i].grade);
        printf("Frequencia: %.2f\n\n", vet[i].attendance);
    }

    // Leitura dos registros para busca:

    // Abre o arquivo binário para leiura
    file = fopen("busca.bin", "rb");
    if (file == NULL)
    {
        printf("Nao foi possivel abrir o arquivo.\n");
        return 1;
    }

    // Le os registros do arquivo e armazena no vetor
    fread(vet_b, sizeof(vet_b), 1, file);

    // Fecha o arquivo
    fclose(file);

    // Exibe os registros lidos para verificar se tudo foi lido corretamente
    for (int i = 0; i < MAX_BUSCA; i++)
    {
        printf("ID Aluno: %s\n", vet_b[i].id_aluno);
        printf("Sigla Disciplina: %s\n", vet_b[i].sigla_disc);
        printf("\n");
    }

    // Inicializa a árvore-B (cria o arquivo de índice se ele não existe)
    initialize_btree(INDEX_FILENAME);

    // Abre o arquivo de índice e o arquivo de dados
    index_file = fopen(INDEX_FILENAME, "rb+");
    /*Header header;
    header = read_header(index_file);
    printf("Root: %d\n", header.root_rrn);
    printf("Insert counter: %d\n", header.insert_count);
    printf("Search counter: %d\n", header.search_count);*/

    data_file = fopen(FILENAME, "rb+");

    if (!data_file)
    {
        data_file = fopen(FILENAME, "wb+");
    }

    char option = 'a';
    while (option != '0')
    {
        printf("\nEscolha uma opcao:\n");
        printf("1. Inserir um aluno\n");
        printf("2. Buscar um aluno\n");
        printf("3. Listar todos os alunos\n");
        printf("0. Sair\n");
        printf("Opcao: ");
        scanf(" %c", &option);

        // if (option == '0')
        // break;

        switch (option)
        {
        case '0':
            break;
        case '1':
        {
            // Insere um novo registro de aluno

            // Le o header
            Header header;
            header = read_header(index_file);

            // printf("Inserindo aluno de numero %d.\n", header.insert_count);

            // Insere o aluno no arquivo e na árvore-B
            insert_student(index_file, data_file, &vet[header.insert_count]);
            break;
        }
        case '2':
        {
            // Busca um aluno específico

            // Le o header
            Header header;
            header = read_header(index_file);

            char key[7];
            memcpy(key, &vet_b[header.search_count], 3);
            memcpy(key + 3, &vet_b[header.search_count], 3);
            key[6] = '\0';

            search_student(index_file, data_file, key);
            break;
        }
        case '3':
        {
            // Lista todos os registros em ordem
            Header header = read_header(index_file);
            list_all_students(data_file, header.root_rrn);
            break;
        }
        default:
            printf("Opcao invalida! Tente novamente.\n");
        }
    }

    // Fecha os arquivos antes de sair
    fclose(index_file);
    fclose(data_file);

    printf("Programa encerrado.\n");
    return 0;
}

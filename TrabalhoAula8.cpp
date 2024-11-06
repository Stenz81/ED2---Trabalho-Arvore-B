/* bt.h
 header file for btree programs
*/
#include <stdio.h>
#include <stdlib.h>

#define MAXKEYS 4
#define MINKEYS MAXKEYS / 2
#define NIL (-1)
#define NOKEY '@'
#define NO 0
#define YES 1

typedef struct
{
    short keycount;           // number of keys in page
    char key[MAXKEYS];        // the actual keys
    short child[MAXKEYS + 1]; // ptrs to rrns of descendants
} BTPAGE;

#define PAGESIZE sizeof(BTPAGE)

short root; // rrn of root page
FILE *btfd; // file descriptor of btree file
FILE *infd; // file descriptor of input file

/* prototypes */
/*
void btclose();
int btopen();
int btread(short rrn, BTPAGE *page_ptr);
int btwrite(short rrn, BTPAGE *page_ptr);
short create_root(char key, short left, short right);
short create_tree();
short getpage();
short getroot();
int insert(short rrn, char key, short *promo_r_child, char *promo_key);
void ins_in_page(char key, short r_child, BTPAGE *p_page);
void pageinit(BTPAGE *p_page);
void putroot(short root);
int search_node(char key, BTPAGE *p_page, short *pos);
void split(char key, short r_child, BTPAGE *p_oldpage, char *promo_key, short *promo_r_child, BTPAGE *p_newpage);
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* btio.c
 Contains btree functions that directly involve file I/O:   */

// #include "bt.h"
// #include "fileio.h"

//FILE *btfd; // global file descriptor for "btree.dat"

// Função para abrir o arquivo "btree.dat" em modo de leitura/escrita binária
int btopen()
{
    btfd = fopen("btree.bin", "rb+");
    if (btfd != NULL) {
        //fclose(btfd);  // Fecha o arquivo, pois só precisamos verificar a abertura
        return 1;      // Retorna 1 para indicar sucesso
    } else {
        return 0;      // Retorna 0 para indicar falha
    }
}

// Função para fechar o arquivo
void btclose()
{
    if (btfd != NULL)
    {
        fclose(btfd);
    }
}

// Função para obter a raiz da árvore B do arquivo
short getroot()
{
    short root;

    fseek(btfd, 0L, SEEK_SET);

    if (fread(&root, sizeof(short), 1, btfd) == 0)
    {
        printf("Error: Unable to get root. \007\n");
        exit(1);
    }
    return root;
}

// Função para atualizar a raiz da árvore B no arquivo
void putroot(short root)
{
    // Posiciona o cursor no início do arquivo
    fseek(btfd, 0L, SEEK_SET);

    // Escreve o valor da raiz no arquivo
    fwrite(&root, sizeof(short), 1, btfd);
}

// Função para obter a página a partir do arquivo
short getpage()
{
    long addr;
    // Vai para o final do arquivo e calcula o endereço da página
    fseek(btfd, 0L, SEEK_END);
    addr = ftell(btfd) - sizeof(short);
    return ((short)addr / PAGESIZE);
}

// Função para ler uma página da árvore B
int btread(short rrn, BTPAGE *page_ptr)
{
    long addr;

    // Calcula o endereço da página e posiciona o cursor
    addr = (long)rrn * (long)PAGESIZE + sizeof(short);
    fseek(btfd, addr, SEEK_SET);

    // Lê a página do arquivo
    return fread(page_ptr, PAGESIZE, 1, btfd) == 1;
}

// Função para escrever uma página na árvore B
int btwrite(short rrn, BTPAGE *page_ptr)
{
    long addr;

    // Calcula o endereço da página e posiciona o cursor
    addr = (long)rrn * (long)PAGESIZE + sizeof(short);
    fseek(btfd, addr, SEEK_SET);

    // Escreve a página no arquivo
    return fwrite(page_ptr, PAGESIZE, 1, btfd) == 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* btutil.c
 Contains utility function for btree program
*/

void pageinit(BTPAGE *p_page)
{
    int j;
    for (j = 0; j < MAXKEYS; j++)
    {
        p_page->key[j] = NOKEY;
        p_page->child[j] = NIL;
    }
    p_page->child[MAXKEYS] = NIL;
}

short create_root(char key, short left, short right)
{
    BTPAGE page;
    short rrn;
    rrn = getpage();
    pageinit(&page);
    page.key[0] = key;
    page.child[0] = left;
    page.child[1] = right;
    page.keycount = 1;
    btwrite(rrn, &page);
    putroot(rrn);
    return (rrn);
}

// Função para criar a árvore B
short create_tree()
{
    char key;

    btfd = fopen("btree.bin", "wb+");
    if (btfd == NULL)
    {
        perror("Erro ao criar o arquivo");
        exit(1);
    }
    btclose();
    btopen();
    printf("Insira o caracter:");
    scanf("%c", &key);
    
    return (create_root(key, NIL, NIL));
}

int search_node(char key, BTPAGE *p_page, short *pos)
{
    int i;

    for (i = 0; i < p_page->keycount && key > p_page->key[i]; i++)
        ;

    *pos = i;

    if (*pos < p_page->keycount && key == p_page->key[*pos])
    {
        return (YES);
    }
    else
    {
        return (NO);
    }
}

void ins_in_page(char key, short r_child, BTPAGE *p_page)
{
    int j;
    for (j = p_page->keycount; key < p_page->key[j - 1] && j > 0; j--)
    {
        p_page->key[j] = p_page->key[j - 1];
        p_page->child[j + 1] = p_page->child[j];
    }
    p_page->keycount++;
    p_page->key[j] = key;
    p_page->child[j + 1] = r_child;
}

void split(char key, short r_child, BTPAGE *p_oldpage, char *promo_key, short *promo_r_child, BTPAGE *p_newpage)
{
    int j;
    short mid;
    char workkeys[MAXKEYS + 1];
    short workchil[MAXKEYS + 2];

    for (j = 0; j < MAXKEYS; j++)
    {
        workkeys[j] = p_oldpage->key[j];
        workchil[j] = p_oldpage->child[j];
    }
    workchil[j] = p_oldpage->child[j];
    for (j = MAXKEYS; key < workkeys[j - 1] && j > 0; j--)
    {
        workkeys[j] = workkeys[j - 1];
        workchil[j + 1] = workchil[j];
    }
    workkeys[j] = key;
    workchil[j + 1] = r_child;

    *promo_r_child = getpage();
    pageinit(p_newpage);
    for (j = 0; j < MINKEYS; j++)
    {
        p_oldpage->key[j] = workkeys[j];
        p_oldpage->child[j] = workchil[j];
        p_newpage->key[j] = workkeys[j + 1 + MINKEYS];
        p_newpage->child[j] = workchil[j + 1 + MINKEYS];
        p_oldpage->key[j + MINKEYS] = NOKEY;
        p_oldpage->child[j + 1 + MINKEYS] = NIL;
    }
    p_oldpage->child[MINKEYS] = workchil[MINKEYS];
    p_newpage->child[MINKEYS] = workchil[j + 1 + MINKEYS];
    p_newpage->keycount = MAXKEYS - MINKEYS;
    p_oldpage->keycount = MINKEYS;
    *promo_key = workkeys[MINKEYS];
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* insert.c
 Contains insert() function to insert a key into a btree.   Calls itself recursively until bottom of tree is reached.   Then insert key node.
 If node is out of room,
 - calls split() to split node
 - promotes middle key and rrn of new node
 */

// #include "bt.h"
int insert(short rrn, char key, short *promo_r_child, char *promo_key)
{
    BTPAGE page,         // current page
        newpage;         // new page created if split occurs
    int found, promoted; // boolean values
    short pos,
        p_b_rrn;  // rrn promoted from below
    char p_b_key; // key promoted from below

    if (rrn == NIL)
    {
        *promo_key = key;
        *promo_r_child = NIL;
        return (YES);
    }
    btread(rrn, &page);
    found = search_node(key, &page, &pos);
    if (found)
    {
        printf("Error: attempt to insert duplicate key: %c \n\007", key);
        return (0);
    }
    promoted = insert(page.child[pos], key, &p_b_rrn, &p_b_key);
    if (!promoted)
    {
        return (NO);
    }
    if (page.keycount < MAXKEYS)
    {
        ins_in_page(p_b_key, p_b_rrn, &page);
        btwrite(rrn, &page);
        return (NO);
    }
    else
    {
        split(p_b_key, p_b_rrn, &page, promo_key, promo_r_child, &newpage);
        btwrite(rrn, &page);
        btwrite(*promo_r_child, &newpage);
        return (YES);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* driver.c
Driver for btree tests
Open or creates b-tree file.
Get next key and calls insert to insert key in tree.
If necessary creates new root.
*/
#include <stdio.h>
// #include "bt.h"

int main()
{
    int promoted;   // boolean: tells if a promotion from below
    short root,     // rrn of root page
        promo_rrn;  // rrn promoted from below
    char promo_key, // key promoted from below
        key;        // next key to insert in tree

    if (btopen())
    {
        printf("Arquivo ja existia.\n");
        root = getroot();
    }
    else
    {
        printf("Arquivo ainda não existia. Gerando...\n");
        root = create_tree();
    }
    while ((key = getchar()) != 'q')
    {
        promoted = insert(root, key, &promo_rrn, &promo_key);
        if (promoted)
        {
            root = create_root(promo_key, root, promo_rrn);
        }
    }
    btclose();
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* CONFIGURACOES */
#define HASH_SIZE 17
#define MAX_NOME 64
#define MAX_PISTA 128

/* ESTRUTURAS */

/* Sala: no da arvore binaria que representa a mansao */
typedef struct Sala {
    char nome[MAX_NOME];
    char pista[MAX_PISTA]; /* se vazia, nao ha pista */
    struct Sala *esq;
    struct Sala *dir;
} Sala;

/* No da BST que armazena pistas coletadas (ordenadas por string) */
typedef struct NoPista {
    char chave[MAX_PISTA];
    struct NoPista *esq;
    struct NoPista *dir;
} NoPista;

/* Entrada da tabela hash: chave = pista, valor = nome do suspeito */
typedef struct HashEntry {
    char *chave;
    char *suspeito;
    struct HashEntry *prox;
} HashEntry;

/* Tabela hash (encadeamento) */
typedef struct {
    HashEntry *v[HASH_SIZE];
} HashTable;

/* DECLARACOES DE FUNCOES */

/* criarSala: cria dinamicamente uma sala com nome e pista (pista pode ser "") */
Sala* criarSala(const char *nome, const char *pista);

/* explorarSalas: navega pela arvore (interativo), coleta pistas ao visitar */
void explorarSalas(Sala *raiz, NoPista **bstPistas, HashTable *ht);

/* inserirPista / adicionarPista: insere pista na BST de forma ordenada (sem duplicar) */
NoPista* inserirPista(NoPista *raiz, const char *pista);

/* inserirNaHash: associa uma pista a um suspeito na tabela hash */
void inserirNaHash(HashTable *ht, const char *pista, const char *suspeito);

/* encontrarSuspeito: retorna o suspeito associado a uma pista (ou NULL) */
char* encontrarSuspeito(HashTable *ht, const char *pista);

/* verificarSuspeitoFinal: verifica se pelo menos duas pistas coletadas apontam para o suspeito */
int verificarSuspeitoFinal(NoPista *raiz, HashTable *ht, const char *suspeito);

/* funcoes auxiliares: criacao/mostrar/listar/limpeza */
void inicializarHash(HashTable *ht);
unsigned int hashString(const char *s);
void listarPistas(NoPista *raiz);
void liberarBST(NoPista *raiz);
void liberarHash(HashTable *ht);
void montarMapa(Sala **raiz, HashTable *ht); /* monta mansao e popula hash */
void mostrarMenuNavegacao();

/* IMPLEMENTACAO */

Sala* criarSala(const char *nome, const char *pista) {
    Sala *s = (Sala*) malloc(sizeof(Sala));
    if (!s) {
        fprintf(stderr, "Erro de memoria\n");
        exit(1);
    }
    strncpy(s->nome, nome, MAX_NOME-1);
    s->nome[MAX_NOME-1] = '\0';
    if (pista && pista[0] != '\0') {
        strncpy(s->pista, pista, MAX_PISTA-1);
        s->pista[MAX_PISTA-1] = '\0';
    } else {
        s->pista[0] = '\0';
    }
    s->esq = s->dir = NULL;
    return s;
}

/* inserirPista
   Insere uma pista na BST de forma ordenada. Nao insere duplicatas.
   Retorna a nova raiz (pode ser a mesma).
*/
NoPista* inserirPista(NoPista *raiz, const char *pista) {
    if (!pista || pista[0] == '\0') return raiz;
    if (!raiz) {
        NoPista *n = (NoPista*) malloc(sizeof(NoPista));
        if (!n) {
            fprintf(stderr, "Erro de memoria\n");
            exit(1);
        }
        strncpy(n->chave, pista, MAX_PISTA-1);
        n->chave[MAX_PISTA-1] = '\0';
        n->esq = n->dir = NULL;
        return n;
    }
    int cmp = strcmp(pista, raiz->chave);
    if (cmp < 0) raiz->esq = inserirPista(raiz->esq, pista);
    else if (cmp > 0) raiz->dir = inserirPista(raiz->dir, pista);
    /* se igual, ja existe -> nao inserir */
    return raiz;
}

/* inicializarHash: zera os ponteiros */
void inicializarHash(HashTable *ht) {
    for (int i = 0; i < HASH_SIZE; i++) ht->v[i] = NULL;
}

unsigned int hashString(const char *s) {
    unsigned long h = 5381;
    int c;
    while ((c = *s++)) h = ((h << 5) + h) + c;
    return (unsigned int)(h % HASH_SIZE);
}

/* inserirNaHash
   Insere mapeamento pista -> suspeito. Se ja existir a chave, sobrescreve o suspeito.
*/
void inserirNaHash(HashTable *ht, const char *pista, const char *suspeito) {
    if (!pista || pista[0] == '\0' || !suspeito) return;
    unsigned int idx = hashString(pista);
    HashEntry *p = ht->v[idx];
    while (p) {
        if (strcmp(p->chave, pista) == 0) {
            free(p->suspeito);
            p->suspeito = strdup(suspeito);
            return;
        }
        p = p->prox;
    }
    /* nao encontrado -> inserir no inicio da lista */
    HashEntry *novo = (HashEntry*) malloc(sizeof(HashEntry));
    novo->chave = strdup(pista);
    novo->suspeito = strdup(suspeito);
    novo->prox = ht->v[idx];
    ht->v[idx] = novo;
}

/* encontrarSuspeito
   Retorna ponteiro para a string do suspeito (dentro da hash) ou NULL.
*/
char* encontrarSuspeito(HashTable *ht, const char *pista) {
    if (!pista) return NULL;
    unsigned int idx = hashString(pista);
    HashEntry *p = ht->v[idx];
    while (p) {
        if (strcmp(p->chave, pista) == 0) return p->suspeito;
        p = p->prox;
    }
    return NULL;
}

/* listarPistas: imprime BST em ordem */
void listarPistas(NoPista *raiz) {
    if (!raiz) return;
    listarPistas(raiz->esq);
    printf(" - %s\n", raiz->chave);
    listarPistas(raiz->dir);
}

/* verificarSuspeitoFinal
   Conta quantas pistas coletadas (na BST) apontam para o suspeito.
   Retorna contador (>=0). O requisito eh: se >= 2 -> acusacao valida.
*/
int verificarSuspeitoFinal(NoPista *raiz, HashTable *ht, const char *suspeito) {
    if (!raiz) return 0;
    int count = 0;
    /* recursao: atravessa a BST e compara cada pista */
    int left = verificarSuspeitoFinal(raiz->esq, ht, suspeito);
    int right = verificarSuspeitoFinal(raiz->dir, ht, suspeito);
    char *s = encontrarSuspeito(ht, raiz->chave);
    if (s && strcmp(s, suspeito) == 0) count = 1;
    return left + right + count;
}

/* liberarBST: libera memoria da BST de pistas */
void liberarBST(NoPista *raiz) {
    if (!raiz) return;
    liberarBST(raiz->esq);
    liberarBST(raiz->dir);
    free(raiz);
}

/* liberarHash: libera todas as entradas da hash */
void liberarHash(HashTable *ht) {
    for (int i = 0; i < HASH_SIZE; i++) {
        HashEntry *p = ht->v[i];
        while (p) {
            HashEntry *t = p;
            p = p->prox;
            free(t->chave);
            free(t->suspeito);
            free(t);
        }
        ht->v[i] = NULL;
    }
}

/* mostrarMenuNavegacao: instrucoes rapidas ao jogador */
void mostrarMenuNavegacao() {
    printf("\nNavegacao: digite 'e' para esquerda, 'd' para direita, 's' para sair da exploracao\n");
}

/* montarMapa
   Constroi manualmente a arvore da mansao e tambem popula a hash
   com as associacoes pista -> suspeito.
*/
void montarMapa(Sala **raiz, HashTable *ht) {
    /* Exemplo simples de mansao (fixo) */
    /*               Hall
                     /   \
                 SalaA   SalaB
                 /  \      \
             SalaC SalaD   SalaE
    */
    Sala *hall = criarSala("Hall", "pegadas molhadas");
    Sala *a = criarSala("SalaA", "fio de cabelo ruivo");
    Sala *b = criarSala("SalaB", "");
    Sala *c = criarSala("SalaC", "marca de fumaça no tapete");
    Sala *d = criarSala("SalaD", "copo com pegadas digitais");
    Sala *e = criarSala("SalaE", "bilhete rasgado");

    hall->esq = a; hall->dir = b;
    a->esq = c; a->dir = d;
    b->dir = e;

    *raiz = hall;

    /* Popula hash: associa cada pista a um suspeito (exemplo) */
    inserirNaHash(ht, "pegadas molhadas", "Sr. Verde");
    inserirNaHash(ht, "fio de cabelo ruivo", "Sra. Rosa");
    inserirNaHash(ht, "marca de fumaça no tapete", "Sr. Azul");
    inserirNaHash(ht, "copo com pegadas digitais", "Sra. Rosa");
    inserirNaHash(ht, "bilhete rasgado", "Sr. Verde");

    /* Repare que Sra. Rosa tem 2 pistas (fio de cabelo, copo com digitais) */
}

/* explorarSalas
   Recebe a raiz da mansao, a BST de pistas (por referencia) e a hash.
   Navegacao eh interativa: 'e' = esquerda, 'd' = direita, 's' = sair.
   Ao visitar uma sala, se houver pista e ela nao estiver na BST, ela e coletada.
*/
void explorarSalas(Sala *raiz, NoPista **bstPistas, HashTable *ht) {
    if (!raiz) return;
    Sala *atual = raiz;
    /* pilha de caminho para poder voltar (opcional): vamos manter um array de ponteiros */
    Sala *stack[128];
    int top = -1;
    int sair = 0;
    char cmd[8];

    printf("Comeca a exploracao da mansao. Voce esta na sala: %s\n", atual->nome);
    while (!sair) {
        /* ao chegar em uma sala, mostrar nome e pista (se existir) */
        printf("\n-- Sala atual: %s\n", atual->nome);
        if (atual->pista[0] != '\0') {
            /* verificar se ja coletada: tentamos inserir (BST evita duplicatas) mas queremos informar ao jogador */
            /* para verificar duplicata, fazemos busca simples: */
            /* buscador simples: iterativo */
            NoPista *aux = *bstPistas;
            int achou = 0;
            while (aux) {
                int cmp = strcmp(atual->pista, aux->chave);
                if (cmp == 0) { achou = 1; break; }
                else if (cmp < 0) aux = aux->esq;
                else aux = aux->dir;
            }
            if (!achou) {
                printf("Voce encontrou uma pista: %s\n", atual->pista);
                *bstPistas = inserirPista(*bstPistas, atual->pista);
            } else {
                printf("Pista presente: %s (ja coletada)\n", atual->pista);
            }
            /* mostra suspeito associado, se houver */
            char *sus = encontrarSuspeito(ht, atual->pista);
            if (sus) printf(" -> Esta pista esta associada ao suspeito: %s\n", sus);
        } else {
            printf("Nenhuma pista nesta sala.\n");
        }

        mostrarMenuNavegacao();
        printf("Comando (e/d/s) ou 'b' para voltar ao andar anterior: ");
        if (scanf("%7s", cmd) != 1) break;

        if (cmd[0] == 's') {
            printf("Saindo da exploracao.\n");
            sair = 1;
            break;
        } else if (cmd[0] == 'e') {
            if (atual->esq) {
                /* empilha atual e vai para esquerda */
                stack[++top] = atual;
                atual = atual->esq;
                printf("Indo para esquerda: %s\n", atual->nome);
            } else {
                printf("Nao ha sala a esquerda.\n");
            }
        } else if (cmd[0] == 'd') {
            if (atual->dir) {
                stack[++top] = atual;
                atual = atual->dir;
                printf("Indo para direita: %s\n", atual->nome);
            } else {
                printf("Nao ha sala a direita.\n");
            }
        } else if (cmd[0] == 'b') {
            if (top >= 0) {
                atual = stack[top--];
                printf("Voltando para: %s\n", atual->nome);
            } else {
                printf("Voce esta na raiz, nao ha onde voltar.\n");
            }
        } else {
            printf("Comando invalido.\n");
        }
    }
}

/* FUNCAO MAIN: monta tudo, chama exploracao, e gerencia a fase de acusacao */
int main() {
    Sala *mansao = NULL;
    NoPista *bstPistas = NULL;
    HashTable ht;
    inicializarHash(&ht);

    montarMapa(&mansao, &ht);

    printf("Bem vindo ao Detective Quest - Capitulo Mestre\n");
    printf("Explore a mansao e colete pistas. Quando sair, voce podera acusar um suspeito.\n");

    explorarSalas(mansao, &bstPistas, &ht);

    /* fase final: listar pistas coletadas */
    printf("\nPistas coletadas:\n");
    if (!bstPistas) {
        printf("Nenhuma pista coletada.\n");
    } else {
        listarPistas(bstPistas);
    }

    /* montar lista de suspeitos possiveis (a partir da hash) para ajudar o jogador */
    printf("\nSuspeitos conhecidos (a partir das pistas):\n");
    /* para simplificar, varremos a hash e imprimimos nomes unicos */
    char *suspeitosUnicos[64];
    int ns = 0;
    for (int i = 0; i < HASH_SIZE; i++) {
        HashEntry *p = ht.v[i];
        while (p) {
            int existe = 0;
            for (int k = 0; k < ns; k++) if (strcmp(suspeitosUnicos[k], p->suspeito) == 0) { existe = 1; break; }
            if (!existe) suspeitosUnicos[ns++] = p->suspeito;
            p = p->prox;
        }
    }
    for (int i = 0; i < ns; i++) printf(" - %s\n", suspeitosUnicos[i]);

    if (ns == 0) printf("Nenhum suspeito cadastrado.\n");

    char escolha[MAX_NOME];
    printf("\nQuem voce acusa? (digite exatamente o nome): ");
    if (scanf(" %63[^\n]", escolha) != 1) {
        printf("Entrada invalida. Encerrando.\n");
    } else {
        int cont = verificarSuspeitoFinal(bstPistas, &ht, escolha);
        if (cont >= 2) {
            printf("\nAcusacao: %s\nHouve %d pistas que apontam para ele(a).\nResultado: ACUSACAO VALIDADA. Caso encerrado.\n", escolha, cont);
        } else {
            printf("\nAcusacao: %s\nHouveram apenas %d pistas que apontam para ele(a).\nResultado: ACUSACAO INSUFICIENTE. Investigacao inconclusiva.\n", escolha, cont);
        }
    }

    /* limpeza basica de memoria */
    liberarBST(bstPistas);
    liberarHash(&ht);

    /* liberar mansao (simples traversal) */
    /* aqui liberamos as salas alocadas em montarMapa */
    /* Como sabiamos as salas criadas, liberamos manualmente */
    /* Numa versao maior, faria um freeArvore recursivo */
    free(mansao->esq->esq); /* SalaC */
    free(mansao->esq->dir); /* SalaD */
    free(mansao->esq);      /* SalaA */
    free(mansao->dir->dir); /* SalaE */
    free(mansao->dir);      /* SalaB */
    free(mansao);           /* Hall */

    printf("\nObrigado por jogar.\n");
    return 0;
}

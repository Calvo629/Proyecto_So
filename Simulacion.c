#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "cabeceras.h" // Incluye las estructuras (superbloque, bytemaps, etc.)

#define SIZE_PARTICION 51200  // Tamaño total de la partición: 512 bytes * 100 bloques
#define SIZE_BLOQUE 512       // Tamaño de un bloque en bytes

// Prototipos de funciones
void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *superbloque);
void Printbytemaps(EXT_BYTE_MAPS *bytemaps);
void Directorio(EXT_ENTRADA_DIR *directorio, EXT_SIMPLE_INODE *inodos);
void DumpHex(const unsigned char *data, size_t size);
int BuscarFichero(EXT_ENTRADA_DIR *directorio, const char *nombre);
void RenombrarArchivo(EXT_ENTRADA_DIR *directorio, EXT_SIMPLE_INODE *inodos,
                      EXT_BYTE_MAPS *bytemaps, EXT_SIMPLE_SUPERBLOCK *superbloque,
                      const char *nombreViejo, const char *nombreNuevo);
void BorrarArchivo(EXT_ENTRADA_DIR *directorio, EXT_SIMPLE_INODE *inodos,
                   EXT_BYTE_MAPS *bytemaps, EXT_SIMPLE_SUPERBLOCK *superbloque,
                   const char *nombre);
void CopiarArchivo(EXT_ENTRADA_DIR *directorio, EXT_SIMPLE_INODE *inodos,
                   EXT_BYTE_MAPS *bytemaps, EXT_SIMPLE_SUPERBLOCK *superbloque,
                   EXT_DATOS *datos, const char *nombreOrigen, const char *nombreDestino);
void ImprimirArchivo(EXT_ENTRADA_DIR *directorio, EXT_SIMPLE_INODE *inodos,
                     EXT_DATOS *datos, const char *nombre);

int main() {
    FILE *fich; // Puntero al archivo binario "particion.bin"
    unsigned char particion[SIZE_PARTICION]; // Array que almacena la partición en memoria

    // Abrir particion.bin en modo lectura/escritura binaria
    fich = fopen("particion.bin", "rb+");
    if (!fich) {
        printf("Error al abrir el archivo particion.bin\n");
        return 1;
    }

    // Leer el contenido de la partición en memoria
    fread(particion, 1, SIZE_PARTICION, fich);
    printf("\n** Sistema de ficheros cargado en memoria **\n");

    // Mapear estructuras a partir del array de memoria
    EXT_SIMPLE_SUPERBLOCK *superbloque = (EXT_SIMPLE_SUPERBLOCK *) particion; 
    EXT_BYTE_MAPS         *bytemaps    = (EXT_BYTE_MAPS *) &particion[SIZE_BLOQUE]; 
    EXT_SIMPLE_INODE      *inodos      = (EXT_SIMPLE_INODE *) &particion[2 * SIZE_BLOQUE];
    EXT_ENTRADA_DIR       *directorio  = (EXT_ENTRADA_DIR *) &particion[3 * SIZE_BLOQUE];
    EXT_DATOS             *datos       = (EXT_DATOS *) &particion[4 * SIZE_BLOQUE];

    char comando[20]; // Variable para almacenar el comando ingresado

    // Bucle principal para procesar comandos
    while (1) {
        // Mostrar opciones disponibles
        printf("\nComandos disponibles:\n");
        printf("  info     -> Muestra información del superbloque\n");
        printf("  dir      -> Lista el contenido del directorio (sin mostrar '.' raíz)\n");
        printf("  bitemaps -> Muestra los bytemaps de bloques e inodos\n");
        printf("  hexa     -> Muestra el bloque de directorio en formato hexadecimal\n");
        printf("  search   -> Busca un fichero por nombre\n");
        printf("  rename   -> Renombra un fichero\n");
        printf("  delete   -> Borra un fichero\n");
        printf("  copy     -> Copia un fichero\n");
        printf("  imprimir -> Imprime el contenido de un fichero\n");
        printf("  salir    -> Finaliza el programa\n");

        // Leer el comando del usuario
        printf("\nIngresa un comando: ");
        scanf("%s", comando);

        // Procesar el comando ingresado
        if (strcmp(comando, "salir") == 0) {
            printf("Saliendo del programa...\n");
            break; // Finaliza el bucle
        } else if (strcmp(comando, "info") == 0) {
            LeeSuperBloque(superbloque);
        } else if (strcmp(comando, "dir") == 0) {
            Directorio(directorio, inodos);
        } else if (strcmp(comando, "bitemaps") == 0) {
            Printbytemaps(bytemaps);
        } else if (strcmp(comando, "hexa") == 0) {
            DumpHex(&particion[3 * SIZE_BLOQUE], SIZE_BLOQUE);
        } else if (strcmp(comando, "search") == 0) {
            char nombre[30];
            printf("\nIntroduce el nombre del fichero a buscar: ");
            scanf("%s", nombre);
            int idx = BuscarFichero(directorio, nombre);
            if (idx >= 0) {
                printf("El fichero '%s' existe en el índice %d (Inodo=%u)\n", 
                       nombre, idx, directorio[idx].dir_inodo);
            } else {
                printf("No existe el fichero '%s' en el directorio.\n", nombre);
            }
        } else if (strcmp(comando, "rename") == 0) {
            char nombreViejo[30], nombreNuevo[30];
            printf("\nIntroduce el nombre actual del fichero: ");
            scanf("%s", nombreViejo);
            printf("Introduce el nuevo nombre: ");
            scanf("%s", nombreNuevo);
            RenombrarArchivo(directorio, inodos, bytemaps, superbloque,
                             nombreViejo, nombreNuevo);
        } else if (strcmp(comando, "delete") == 0) {
            char nombre[30];
            printf("\nIntroduce el nombre del fichero a borrar: ");
            scanf("%s", nombre);
            BorrarArchivo(directorio, inodos, bytemaps, superbloque, nombre);
        } else if (strcmp(comando, "copy") == 0) {
            char nombreOrigen[30], nombreDestino[30];
            printf("\nIntroduce el nombre del fichero origen: ");
            scanf("%s", nombreOrigen);
            printf("Introduce el nombre del fichero destino: ");
            scanf("%s", nombreDestino);
            CopiarArchivo(directorio, inodos, bytemaps, superbloque,
                          datos, nombreOrigen, nombreDestino);
        } else if (strcmp(comando, "imprimir") == 0) {
            char nombre[30];
            printf("\nIntroduce el nombre del fichero a imprimir: ");
            scanf("%s", nombre);
            ImprimirArchivo(directorio, inodos, datos, nombre);
        } else {
            printf("Comando desconocido: %s\n", comando);
        }
    }

    // Antes de salir, guardamos la partición (por si hubo cambios)
    fseek(fich, 0, SEEK_SET);
    fwrite(particion, 1, SIZE_PARTICION, fich);

    // Cerrar el archivo binario antes de salir
    fclose(fich);
    return 0;
}

//--------------------------------------------------------------------------
// Implementaciones de funciones auxiliares
//--------------------------------------------------------------------------

void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *superbloque) {
    printf("\nInformación del Superbloque:\n");
    printf("  Inodos totales: %u\n", superbloque->s_inodes_count);
    printf("  Bloques totales: %u\n", superbloque->s_blocks_count);
    printf("  Bloques libres: %u\n", superbloque->s_free_blocks_count);
    printf("  Inodos libres: %u\n", superbloque->s_free_inodes_count);
    printf("  Primer bloque de datos: %u\n", superbloque->s_first_data_block);
    printf("  Tamaño del bloque: %u bytes\n", superbloque->s_block_size);
}

void Printbytemaps(EXT_BYTE_MAPS *bytemaps) {
    printf("\nBytemap de bloques (primeros 25):\n");
    for (int i = 0; i < 25; i++) {
        printf("%d ", bytemaps->bmap_bloques[i]);
    }
    printf("\n\nBytemap de inodos:\n");
    for (int i = 0; i < MAX_INODOS; i++) {
        printf("%d ", bytemaps->bmap_inodos[i]);
    }
    printf("\n");
}

void Directorio(EXT_ENTRADA_DIR *directorio, EXT_SIMPLE_INODE *inodos) {
    printf("\nContenido del Directorio:\n");
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (directorio[i].dir_inodo != NULL_INODO) {
            if (strcmp(directorio[i].dir_nfich, ".") == 0) {
                continue; // No mostrar el directorio raíz
            }
            printf("  Nombre: %s\n", directorio[i].dir_nfich);
            printf("  Inodo: %u\n", directorio[i].dir_inodo);
            printf("  Tamaño: %u bytes\n", inodos[directorio[i].dir_inodo].size_fichero);
            printf("  Bloques: ");
            for (int j = 0; j < MAX_NUMS_BLOQUE_INODO; j++) {
                if (inodos[directorio[i].dir_inodo].i_nbloque[j] != NULL_BLOQUE) {
                    printf("%u ", inodos[directorio[i].dir_inodo].i_nbloque[j]);
                }
            }
            printf("\n\n");
        }
    }
}

void DumpHex(const unsigned char *data, size_t size) {
    printf("\nContenido en Hexadecimal:\n");
    for (size_t i = 0; i < size; i += 16) {
        printf("%08X  ", (unsigned int)i);
        for (size_t j = 0; j < 16 && (i + j) < size; j++) {
            printf("%02X ", data[i + j]);
        }
        printf("\n");
    }
}

int BuscarFichero(EXT_ENTRADA_DIR *directorio, const char *nombre) {
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (directorio[i].dir_inodo != NULL_INODO && strcmp(directorio[i].dir_nfich, nombre) == 0) {
            return i; // Fichero encontrado
        }
    }
    return -1; // No encontrado
}

void RenombrarArchivo(EXT_ENTRADA_DIR *directorio, EXT_SIMPLE_INODE *inodos,
                      EXT_BYTE_MAPS *bytemaps, EXT_SIMPLE_SUPERBLOCK *superbloque,
                      const char *nombreViejo, const char *nombreNuevo) {
    int idx = BuscarFichero(directorio, nombreViejo);
    if (idx < 0) {
        printf("Error: No existe el fichero '%s'.\n", nombreViejo);
        return;
    }
    if (BuscarFichero(directorio, nombreNuevo) >= 0) {
        printf("Error: Ya existe un fichero con el nombre '%s'.\n", nombreNuevo);
        return;
    }
    strncpy(directorio[idx].dir_nfich, nombreNuevo, LEN_NFICH - 1);
    directorio[idx].dir_nfich[LEN_NFICH - 1] = '\0'; // Asegurar terminación nula
    printf("Fichero '%s' renombrado a '%s'.\n", nombreViejo, nombreNuevo);
}

void BorrarArchivo(EXT_ENTRADA_DIR *directorio, EXT_SIMPLE_INODE *inodos,
                   EXT_BYTE_MAPS *bytemaps, EXT_SIMPLE_SUPERBLOCK *superbloque,
                   const char *nombre) {
    int idx = BuscarFichero(directorio, nombre);
    if (idx < 0) {
        printf("Error: No existe el fichero '%s'.\n", nombre);
        return;
    }
    unsigned short inodoNum = directorio[idx].dir_inodo;
    for (int j = 0; j < MAX_NUMS_BLOQUE_INODO; j++) {
        if (inodos[inodoNum].i_nbloque[j] != NULL_BLOQUE) {
            unsigned short bloque = inodos[inodoNum].i_nbloque[j];
            bytemaps->bmap_bloques[bloque] = 0; // Liberar bloque
            superbloque->s_free_blocks_count++;
            inodos[inodoNum].i_nbloque[j] = NULL_BLOQUE;
        }
    }
    bytemaps->bmap_inodos[inodoNum] = 0;
    superbloque->s_free_inodes_count++;
    directorio[idx].dir_inodo = NULL_INODO;
    strcpy(directorio[idx].dir_nfich, "");
    printf("Fichero '%s' eliminado correctamente.\n", nombre);
}

void CopiarArchivo(EXT_ENTRADA_DIR *directorio, EXT_SIMPLE_INODE *inodos,
                   EXT_BYTE_MAPS *bytemaps, EXT_SIMPLE_SUPERBLOCK *superbloque,
                   EXT_DATOS *datos, const char *nombreOrigen, const char *nombreDestino) {
    int idxOrigen = BuscarFichero(directorio, nombreOrigen);
    if (idxOrigen < 0) {
        printf("Error: No existe el fichero origen '%s'.\n", nombreOrigen);
        return;
    }
    if (BuscarFichero(directorio, nombreDestino) >= 0) {
        printf("Error: Ya existe un fichero con el nombre '%s'.\n", nombreDestino);
        return;
    }
    int idxDestino = -1;
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (directorio[i].dir_inodo == NULL_INODO) {
            idxDestino = i;
            break;
        }
    }
    if (idxDestino == -1) {
        printf("Error: No hay espacio en el directorio para el nuevo fichero.\n");
        return;
    }
    int inodoDestino = -1;
    for (int i = 0; i < MAX_INODOS; i++) {
        if (bytemaps->bmap_inodos[i] == 0) {
            inodoDestino = i;
            break;
        }
    }
    if (inodoDestino == -1) {
        printf("Error: No hay inodos libres disponibles.\n");
        return;
    }
    unsigned short inodoOrigen = directorio[idxOrigen].dir_inodo;
    inodos[inodoDestino].size_fichero = inodos[inodoOrigen].size_fichero;
    for (int j = 0; j < MAX_NUMS_BLOQUE_INODO; j++) {
        if (inodos[inodoOrigen].i_nbloque[j] != NULL_BLOQUE) {
            unsigned short bloqueOrigen = inodos[inodoOrigen].i_nbloque[j];
            int bloqueDestino = -1;
            for (int k = PRIM_BLOQUE_DATOS; k < MAX_BLOQUES_PARTICION; k++) {
                if (bytemaps->bmap_bloques[k] == 0) {
                    bloqueDestino = k;
                    break;
                }
            }
            if (bloqueDestino == -1) {
                printf("Error: No hay bloques libres disponibles.\n");
                return;
            }
            bytemaps->bmap_bloques[bloqueDestino] = 1;
            superbloque->s_free_blocks_count--;
            memcpy(&datos[bloqueDestino], &datos[bloqueOrigen], SIZE_BLOQUE);
            inodos[inodoDestino].i_nbloque[j] = bloqueDestino;
        } else {
            inodos[inodoDestino].i_nbloque[j] = NULL_BLOQUE;
        }
    }
    bytemaps->bmap_inodos[inodoDestino] = 1;
    superbloque->s_free_inodes_count--;
    strcpy(directorio[idxDestino].dir_nfich, nombreDestino);
    directorio[idxDestino].dir_inodo = inodoDestino;
        strcpy(directorio[idxDestino].dir_nfich, nombreDestino);
    directorio[idxDestino].dir_inodo = inodoDestino;
    printf("Fichero '%s' copiado exitosamente a '%s'.\n", nombreOrigen, nombreDestino);
}

void ImprimirArchivo(EXT_ENTRADA_DIR *directorio, EXT_SIMPLE_INODE *inodos,
                     EXT_DATOS *datos, const char *nombre) {
    int idx = BuscarFichero(directorio, nombre);
    if (idx < 0) {
        printf("Error: El fichero '%s' no existe.\n", nombre);
        return;
    }
    unsigned short inodoNum = directorio[idx].dir_inodo;
    if (inodoNum == NULL_INODO) {
        printf("Error: El fichero '%s' no tiene un inodo válido.\n", nombre);
        return;
    }
    printf("Contenido del fichero '%s':\n", nombre);
    for (int i = 0; i < MAX_NUMS_BLOQUE_INODO; i++) {
        if (inodos[inodoNum].i_nbloque[i] != NULL_BLOQUE) {
            unsigned short bloque = inodos[inodoNum].i_nbloque[i];
            printf("%.*s", SIZE_BLOQUE, (char *)&datos[bloque]);
        }
    }
    printf("\n");
}

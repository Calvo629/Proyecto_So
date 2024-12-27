#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "cabeceras.h" // Incluye las estructuras (superbloque, bytemaps, etc.)

#define SIZE_PARTICION 51200  // Tamaño total de la partición: 512 bytes * 100 bloques
#define SIZE_BLOQUE 512       // Tamaño de un bloque en bytes

// Prototipos de funciones existentes
void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *superbloque);
void Printbytemaps(EXT_BYTE_MAPS *bytemaps);
void Directorio(EXT_ENTRADA_DIR *directorio, EXT_SIMPLE_INODE *inodos);
void DumpHex(const unsigned char *data, size_t size);

// Prototipos de las nuevas funciones
int  BuscarFichero(EXT_ENTRADA_DIR *directorio, const char *nombre);
void RenombrarArchivo(EXT_ENTRADA_DIR *directorio, EXT_SIMPLE_INODE *inodos,
                      EXT_BYTE_MAPS *bytemaps, EXT_SIMPLE_SUPERBLOCK *superbloque,
                      const char *nombreViejo, const char *nombreNuevo);
void BorrarArchivo(EXT_ENTRADA_DIR *directorio, EXT_SIMPLE_INODE *inodos,
                   EXT_BYTE_MAPS *bytemaps, EXT_SIMPLE_SUPERBLOCK *superbloque,
                   const char *nombre);

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
    printf("\n*** Sistema de ficheros cargado en memoria ***\n");

    // Mapear estructuras a partir del array de memoria
    EXT_SIMPLE_SUPERBLOCK *superbloque = (EXT_SIMPLE_SUPERBLOCK *) particion; 
    EXT_BYTE_MAPS         *bytemaps    = (EXT_BYTE_MAPS *) &particion[SIZE_BLOQUE]; 
    EXT_SIMPLE_INODE      *inodos      = (EXT_SIMPLE_INODE *) &particion[2 * SIZE_BLOQUE];
    EXT_ENTRADA_DIR       *directorio  = (EXT_ENTRADA_DIR *) &particion[3 * SIZE_BLOQUE];

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
        printf("  salir    -> Finaliza el programa\n");

        // Leer el comando del usuario
        printf("\nIngresa un comando: ");
        scanf("%s", comando);

        // Procesar el comando ingresado
        if (strcmp(comando, "salir") == 0) {
            printf("Saliendo del programa...\n");
            break; // Finaliza el bucle
        }

        // Usar "if/else if" (o un switch con una función que mapee strings a enum) para los comandos
        if (strcmp(comando, "info") == 0) {
            printf("\n====================================\n");
            printf("Comando: info (Información del superbloque)\n");
            printf("====================================\n");
            LeeSuperBloque(superbloque);

        } else if (strcmp(comando, "dir") == 0) {
            printf("\n====================================\n");
            printf("Comando: dir (Contenido del directorio)\n");
            printf("====================================\n");
            Directorio(directorio, inodos);

        } else if (strcmp(comando, "bitemaps") == 0) {
            printf("\n====================================\n");
            printf("Comando: bitemaps (Bytemap de bloques e inodos)\n");
            printf("====================================\n");
            Printbytemaps(bytemaps);

        } else if (strcmp(comando, "hexa") == 0) {
            printf("\n====================================\n");
            printf("Comando: hexa (Bloque de directorio en formato hexadecimal)\n");
            printf("====================================\n");
            DumpHex(&particion[3 * SIZE_BLOQUE], SIZE_BLOQUE);

        } else if (strcmp(comando, "search") == 0) {
            // Buscar fichero
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
            // Renombrar fichero
            char nombreViejo[30], nombreNuevo[30];
            printf("\nIntroduce el nombre actual del fichero: ");
            scanf("%s", nombreViejo);
            printf("Introduce el nuevo nombre: ");
            scanf("%s", nombreNuevo);

            RenombrarArchivo(directorio, inodos, bytemaps, superbloque,
                             nombreViejo, nombreNuevo);

        } else if (strcmp(comando, "delete") == 0) {
            // Borrar fichero
            char nombre[30];
            printf("\nIntroduce el nombre del fichero a borrar: ");
            scanf("%s", nombre);

            BorrarArchivo(directorio, inodos, bytemaps, superbloque, nombre);

        } else {
            // Comando desconocido
            printf("Comando desconocido: %s\n", comando);
            printf("Por favor, ingresa un comando válido.\n");
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
// Funciones básicas (ya existían)
//--------------------------------------------------------------------------
void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *superbloque) {
    printf("Número total de inodos: %u\n",  superbloque->s_inodes_count);
    printf("Número total de bloques: %u\n", superbloque->s_blocks_count);
    printf("Bloques libres: %u\n",         superbloque->s_free_blocks_count);
    printf("Inodos libres: %u\n",          superbloque->s_free_inodes_count);
    printf("Primer bloque de datos: %u\n", superbloque->s_first_data_block);
    printf("Tamaño del bloque: %u bytes\n",superbloque->s_block_size);
}

void Printbytemaps(EXT_BYTE_MAPS *bytemaps) {
    printf("Bytemap de bloques (primeros 25):\n");
    for (int i = 0; i < 25; i++) {
        printf("%d ", bytemaps->bmap_bloques[i]);
    }
    printf("\n");

    printf("Bytemap de inodos:\n");
    for (int i = 0; i < MAX_INODOS; i++) {
        printf("%d ", bytemaps->bmap_inodos[i]);
    }
    printf("\n");
}

void Directorio(EXT_ENTRADA_DIR *directorio, EXT_SIMPLE_INODE *inodos) {
    printf("Archivos en el directorio:\n");
    int encontrados = 0;

    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (directorio[i].dir_inodo != NULL_INODO) {
            // Saltar la entrada del directorio raíz ('.')
            if (strcmp(directorio[i].dir_nfich, ".") == 0) {
                continue;
            }
            encontrados++;
            printf("\nArchivo %d:\n", i);
            printf("  Nombre: %s\n", directorio[i].dir_nfich);
            printf("  Inodo: %u\n", directorio[i].dir_inodo);
            printf("  Tamaño: %u bytes\n", inodos[directorio[i].dir_inodo].size_fichero);

            printf("  Bloques: ");
            for (int j = 0; j < MAX_NUMS_BLOQUE_INODO; j++) {
                if (inodos[directorio[i].dir_inodo].i_nbloque[j] != NULL_BLOQUE) {
                    printf("%u ", inodos[directorio[i].dir_inodo].i_nbloque[j]);
                }
            }
            printf("\n");
        }
    }
    if (encontrados == 0) {
        printf("  (No hay archivos o solo está la entrada '.')\n");
    }
}

void DumpHex(const unsigned char *data, size_t size) {
    printf("Offset    Hexadecimal                                       ASCII\n");
    printf("---------------------------------------------------------------------\n");

    for (size_t i = 0; i < size; i += 16) {
        printf("%08X  ", (unsigned int)i); // Offset en hexadecimal

        // Imprimir los valores en hexadecimal
        for (size_t j = 0; j < 16; j++) {
            if (i + j < size) {
                printf("%02X ", data[i + j]);
            } else {
                printf("   "); // Relleno para alineación
            }
            if (j == 7) printf(" "); // Espacio adicional en medio
        }

        printf(" ");

        // Imprimir los valores en ASCII
        for (size_t j = 0; j < 16; j++) {
            if (i + j < size) {
                unsigned char c = data[i + j];
                printf("%c", isprint(c) ? c : '.'); // Mostrar solo caracteres imprimibles
            }
        }
        printf("\n");
    }
}

//--------------------------------------------------------------------------
// Nuevas funciones: buscar, renombrar, borrar
//--------------------------------------------------------------------------

/**
 * @brief Busca un fichero por nombre en el directorio.
 * @param directorio Puntero al array de entradas de directorio.
 * @param nombre Nombre del fichero a buscar.
 * @return Índice del fichero en el directorio, o -1 si no existe.
 */
int BuscarFichero(EXT_ENTRADA_DIR *directorio, const char *nombre) {
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (directorio[i].dir_inodo != NULL_INODO) {
            if (strcmp(directorio[i].dir_nfich, nombre) == 0) {
                return i; // Fichero encontrado
            }
        }
    }
    return -1; // No encontrado
}

/**
 * @brief Renombra un fichero en el directorio (si existe).
 * @param directorio Puntero al array de entradas de directorio.
 * @param inodos Puntero al array de inodos (por si hiciera falta algún ajuste).
 * @param bytemaps Puntero a la estructura de mapas de bits (por si hiciera falta).
 * @param superbloque Puntero al superbloque (por si hiciera falta).
 * @param nombreViejo Nombre actual del fichero.
 * @param nombreNuevo Nuevo nombre a asignar.
 */
void RenombrarArchivo(EXT_ENTRADA_DIR *directorio, EXT_SIMPLE_INODE *inodos,
                      EXT_BYTE_MAPS *bytemaps, EXT_SIMPLE_SUPERBLOCK *superbloque,
                      const char *nombreViejo, const char *nombreNuevo)
{
    // Buscar el fichero
    int idx = BuscarFichero(directorio, nombreViejo);
    if (idx < 0) {
        printf("Error: No existe el fichero '%s'\n", nombreViejo);
        return;
    }

    // Verificar que el nuevo nombre no exceda la capacidad
    if (strlen(nombreNuevo) >= sizeof(directorio[idx].dir_nfich)) {
        printf("Error: El nuevo nombre '%s' es demasiado largo.\n", nombreNuevo);
        return;
    }

    // Renombrar (copiar la cadena)
    strcpy(directorio[idx].dir_nfich, nombreNuevo);
    printf("Fichero '%s' renombrado a '%s' exitosamente.\n", nombreViejo, nombreNuevo);

    // (Opcional) Si necesitas actualizar algo en inodos, bytemaps o superbloque, hazlo aquí.
    // En este ejemplo, no es necesario.
}

/**
 * @brief Borra un fichero: libera su inodo y bloques, y actualiza bytemaps y superbloque.
 * @param directorio Puntero al array de entradas de directorio.
 * @param inodos Puntero al array de inodos.
 * @param bytemaps Puntero a la estructura de mapas de bits (inodos, bloques).
 * @param superbloque Puntero a la estructura de superbloque.
 * @param nombre Nombre del fichero a borrar.
 */
void BorrarArchivo(EXT_ENTRADA_DIR *directorio, EXT_SIMPLE_INODE *inodos,
                   EXT_BYTE_MAPS *bytemaps, EXT_SIMPLE_SUPERBLOCK *superbloque,
                   const char *nombre)
{
    // Buscar el fichero
    int idx = BuscarFichero(directorio, nombre);
    if (idx < 0) {
        printf("Error: No existe el fichero '%s'\n", nombre);
        return;
    }

    unsigned short inodoNum = directorio[idx].dir_inodo;
    if (inodoNum == NULL_INODO) {
        printf("Error: El fichero '%s' ya está borrado.\n", nombre);
        return;
    }

    // Liberar los bloques asignados a ese inodo
    for (int j = 0; j < MAX_NUMS_BLOQUE_INODO; j++) {
        unsigned short nbloque = inodos[inodoNum].i_nbloque[j];
        if (nbloque != NULL_BLOQUE) {
            // Liberar el bloque en bmap_bloques (marca a 0)
            // Ajusta si tu array bmap_bloques es más grande que 25
            if (nbloque < 25) {
                bytemaps->bmap_bloques[nbloque] = 0; 
                superbloque->s_free_blocks_count++;
            }
            inodos[inodoNum].i_nbloque[j] = NULL_BLOQUE; 
        }
    }

    // Eliminar la referencia del directorio (marcar como vacío)
    directorio[idx].dir_inodo = NULL_INODO;
    strcpy(directorio[idx].dir_nfich, "");

    // Liberar el inodo en bmap_inodos
    if (inodoNum < MAX_INODOS) {
        bytemaps->bmap_inodos[inodoNum] = 0;
        superbloque->s_free_inodes_count++;
    }

    // Dejar el inodo “limpio” por si se usa más adelante
    inodos[inodoNum].size_fichero = 0;

    printf("Fichero '%s' borrado exitosamente.\n", nombre);
}
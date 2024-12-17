#include <stdio.h>
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
    EXT_BYTE_MAPS *bytemaps = (EXT_BYTE_MAPS *) &particion[SIZE_BLOQUE]; 
    EXT_SIMPLE_INODE *inodos = (EXT_SIMPLE_INODE *) &particion[2 * SIZE_BLOQUE];
    EXT_ENTRADA_DIR *directorio = (EXT_ENTRADA_DIR *) &particion[3 * SIZE_BLOQUE];

    // Ejecución de comandos
    printf("\n====================================\n");
    printf("Comando: info (Información del superbloque)\n");
    printf("====================================\n");
    LeeSuperBloque(superbloque);

    printf("\n====================================\n");
    printf("Comando: bytemaps (Bytemap de bloques e inodos)\n");
    printf("====================================\n");
    Printbytemaps(bytemaps);

    printf("\n====================================\n");
    printf("Comando: dir (Contenido del directorio)\n");
    printf("====================================\n");
    Directorio(directorio, inodos);

    printf("\n====================================\n");
    printf("Depuración: Bloque de directorio en formato hexadecimal\n");
    printf("====================================\n");
    DumpHex(&particion[3 * SIZE_BLOQUE], SIZE_BLOQUE);

    // Cerrar el archivo binario
    fclose(fich);
    return 0;
}

// Función para mostrar información del superbloque
void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *superbloque) {
    printf("Número total de inodos: %u\n", superbloque->s_inodes_count);
    printf("Número total de bloques: %u\n", superbloque->s_blocks_count);
    printf("Bloques libres: %u\n", superbloque->s_free_blocks_count);
    printf("Inodos libres: %u\n", superbloque->s_free_inodes_count);
    printf("Primer bloque de datos: %u\n", superbloque->s_first_data_block);
    printf("Tamaño del bloque: %u bytes\n", superbloque->s_block_size);
}

// Función para imprimir los bytemaps de bloques e inodos
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

// Función para listar el contenido del directorio (sin mostrar el directorio raíz '.')
void Directorio(EXT_ENTRADA_DIR *directorio, EXT_SIMPLE_INODE *inodos) {
    printf("Archivos en el directorio:\n");
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (directorio[i].dir_inodo != NULL_INODO) { // Comprueba si la entrada está en uso
            // Saltar la entrada del directorio raíz ('.')
            if (strcmp(directorio[i].dir_nfich, ".") == 0) {
                continue; // Salta a la siguiente iteración
            }

            // Mostrar el resto de los archivos
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
}

// Función para imprimir los datos en formato hexadecimal formateado
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

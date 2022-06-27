/**
 * Aluno: Raphael Abreu Farias de Jesus
 * Matrícula: 20150460
 * Trabalho: Desenvolver um sistema de arquivos que simula padrão EXT3. O sistema deve ser desenvolvido em C ou C++ utilizando o compilador GNU GCC e chamadas de sistemas do padrão POSIX.
 * Disciplina: Arquitetura de Sistemas Operacionais - DEC7556
 * Turma: 07655
 * Data de entrega: 03/07/2022
 */

/**
 * Implemente aqui as funções dos sistema de arquivos que simula EXT3
 */

#include "fs.h"
#include <string.h>
#include <math.h>

void initFs(std::string fsFileName, int blockSize, int numBlocks, int numInodes)
{
    FILE *file = fopen(fsFileName.c_str(), "w+");

    size_t sizeOfChar = sizeof(char);        // Size of a char
    fwrite(&blockSize, sizeOfChar, 1, file); // Byte 1 - Size of the block
    fwrite(&numBlocks, sizeOfChar, 1, file); // Byte 2 - Number of blocks
    fwrite(&numInodes, sizeOfChar, 1, file); // Byte 3 - Number of Inodes

    char firstByteOfMap = 0x01;                   // First block is used by root
    fwrite(&firstByteOfMap, sizeOfChar, 1, file); // Byte 4 - Define block 0 as used because of root (first bit of Bitmap)

    char sizeOfBitmap = (numBlocks - 1) / 8;                 // Size of the Bitmap (-1 because of root)
    int sizeOfInodeVector = (numInodes - 1) * sizeof(INODE); // Size of the Inode Vector (-1 because of root)
    char empty[sizeOfInodeVector];                           // Char vector filled with 0 to set 0 values on fwrite

    for (int i = 0; i < sizeof(empty); ++i)
    {
        empty[i] = 0;
    }

    fwrite(&empty, sizeOfChar, sizeOfBitmap, file); // Byte 5 to Byte(5 + bitmap size -1) - Fill the rest of bitmap with 0 (all bits of bitmap)

    INODE root = {
        // First inode is the directory "/"
        0x01,               // 0x01 if used, 0x00 if free
        0x01,               // 0x01 if directory, 0x00 if file
        "/",                // Name of file/directory
        0x00,               // Size of file/directory in bytes
        {0x00, 0x00, 0x00}, // Direct blocks
        {0x00, 0x00, 0x00}, // Indirect blocks
        {0x00, 0x00, 0x00}, // Double indirect blocks
    };

    fwrite(&root, sizeOfChar, sizeof(INODE), file);      // Add root inode in file
    fwrite(&empty, sizeOfChar, sizeOfInodeVector, file); // Fill the rest of inodes with 0
    fwrite(&empty, sizeOfChar, 1, file);                 // Index of root

    int sizeOfBlockVector = numBlocks * blockSize;       // Size of the Block Vector
    fwrite(&empty, sizeOfChar, sizeOfBlockVector, file); // Fill the block vector wuth 0 (because it's empty)

    fclose(file);
}

/**
 * @brief Adiciona um novo arquivo dentro do sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
 * @param fsFileName arquivo que contém um sistema sistema de arquivos que simula EXT3.
 * @param filePath caminho completo novo arquivo dentro sistema de arquivos que simula EXT3.
 * @param fileContent conteúdo do novo arquivo
 */
void addFile(std::string fsFileName, std::string filePath, std::string fileContent)
{
    FILE *file = fopen(fsFileName.c_str(), "r+");

    char blockSize, numBlocks, numInodes, tempValues[3];
    size_t sizeOfChar = sizeof(char); // Size of a char

    fread(&tempValues, sizeOfChar, 3, file);
    blockSize = tempValues[0];
    numBlocks = tempValues[1];
    numInodes = tempValues[2];

    char sizeOfBitmap = ceil(numBlocks / 8);           // Size of the Bitmap (-1 because of root)
    int sizeOfInodeVector = numInodes * sizeof(INODE); // Size of the Inode Vector (-1 because of root)
    int sizeOfBlockVector = numBlocks * blockSize;     // Size of the Block Vector

    int sizeOfFileSystem = sizeOfBitmap + sizeOfInodeVector + 1 + sizeOfBlockVector;

    char fileSystem[sizeOfFileSystem];

    fread(&fileSystem, sizeOfChar, sizeOfFileSystem, file);

    int goTo = 3 + sizeOfBitmap + 22;

    fseek(file, goTo, SEEK_SET);

    int i = 0;
    printf("filePath:\n\"");
    char aqui[10];
    while (i < strlen(filePath.c_str()))
    {
        aqui[i] = filePath.at(i);
        printf("%c", filePath.at(i));
        i++;
    }

    const char *fileName = filePath.c_str();

    // INODE inode = {
    //     0x01,               // 0x01 if used, 0x00 if free
    //     0x00,               // 0x01 if directory, 0x00 if file
    //    { &aqui},                // Name of file/directory
    //     0x00,               // Size of file/directory in bytes
    //     {0x00, 0x00, 0x00}, // Direct blocks
    //     {0x00, 0x00, 0x00}, // Indirect blocks
    //     {0x00, 0x00, 0x00}, // Double indirect blocks
    // };

    // free(fileName);

    fwrite(&aqui, sizeOfChar, 10, file);

    printf("\n----------------------\n----------------------\nTamanho do bloco: %u\nNumero de blocos: %u\nNumero de inodes: %u\n----------------------\n----------------------\n", blockSize, numBlocks, numInodes);
    printf("\nArquivo de sistema (%d):\n", sizeof(fileSystem));
    for (size_t i = 0; i < sizeof(fileSystem); i++)
    {
        // if (fileSystem[i] == '/')
        // {
        //     printf("/ ");
        // }
        // else
        printf("%x ", fileSystem[i]);
    }
    printf("\n-------------------");

    // printf("\"\n");

    // i = 0;
    // printf("fileContent:\n\"");
    // while (i < strlen(fileContent.c_str()))
    // {
    //     printf("%c", fileContent.at(i));
    //     i++;
    // }
    // printf("\"\n");

    // i = 0;
    // printf("fsFileName:\n\"");
    // while (i < strlen(fsFileName.c_str()))
    // {
    //     printf("%c", fsFileName.at(i));
    //     i++;
    // }
    // printf("\"\n");
    ///////////////

    // char teste[3];
    //  int inteiro =   fread(&teste, sizeof(char), 3,file);
    // printf("\n--------------\n.......\n%s\n%d\n.......\n-------------\n",teste, inteiro);

    // for (size_t i = 0; i < 3; i++)
    // {
    //     printf("| %d  -  %u |\n",teste[i],teste[i]);
    // }

    //     /////////////

    fclose(file);
}

/**
 * @brief Adiciona um novo diretório dentro do sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
 * @param fsFileName arquivo que contém um sistema sistema de arquivos que simula EXT3.
 * @param dirPath caminho completo novo diretório dentro sistema de arquivos que simula EXT3.
 */
void addDir(std::string fsFileName, std::string dirPath)
{
}

/**
 * @brief Remove um arquivo ou diretório (recursivamente) de um sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
 * @param fsFileName arquivo que contém um sistema sistema de arquivos que simula EXT3.
 * @param path caminho completo do arquivo ou diretório a ser removido.
 */
void remove(std::string fsFileName, std::string path)
{
}

/**
 * @brief Move um arquivo ou diretório em um sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
 * @param fsFileName arquivo que contém um sistema sistema de arquivos que simula EXT3.
 * @param oldPath caminho completo do arquivo ou diretório a ser movido.
 * @param newPath novo caminho completo do arquivo ou diretório.
 */
void move(std::string fsFileName, std::string oldPath, std::string newPath)
{
}

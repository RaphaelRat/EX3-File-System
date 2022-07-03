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
#include "math.h"
#include <string.h>
#include <bitset>
#include <cassert>
#include <iostream>
#include <vector>
#include <sstream>

void initFs(std::string fsFileName, int blockSize, int numBlocks, int numInodes)
{
    FILE *file = fopen(fsFileName.c_str(), "w+");

    size_t sizeOfChar = sizeof(char);        // Size of a char
    fwrite(&blockSize, sizeOfChar, 1, file); // Byte 1 - Size of the block
    fwrite(&numBlocks, sizeOfChar, 1, file); // Byte 2 - Number of blocks
    fwrite(&numInodes, sizeOfChar, 1, file); // Byte 3 - Number of Inodes

    char bitMap = 0x01;                   // First block is used by root
    fwrite(&bitMap, sizeOfChar, 1, file); // Byte 4 - Define block 0 as used because of root (first bit of Bitmap)

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

    fread(&tempValues, sizeOfChar, 3, file); // Read block size, number of blocks and number of inodes
    blockSize = tempValues[0];
    numBlocks = tempValues[1];
    numInodes = tempValues[2];

    char sizeOfBitmap = ceil(numBlocks / 8.0);         // Size of the Bitmap
    int sizeOfInodeVector = numInodes * sizeof(INODE); // Size of the Inode Vector
    int sizeOfBlockVector = numBlocks * blockSize;     // Size of the Block Vector

    // Find first empty inode
    INODE inode;
    int freeInodeIndex; // Index of a free inode
    int goTo = 3 + sizeOfBitmap;
    fseek(file, goTo, SEEK_SET);
    for (int i = 0; i < numInodes; i++)
    {
        fread(&inode, sizeof(INODE), 1, file); // Read inode to find an empty one

        if (inode.IS_USED == 0)
        {
            freeInodeIndex = i;
            break;
        }
    }

    int directoriesCounter = 0, slashIndex = 0; // Number of directories
    char directoryName[10], pathName[10];       // Name of the path and directory

    for (size_t i = 0; i < strlen(filePath.c_str()); i++)
    {
        if (filePath.at(i) == '/')
        {
            directoriesCounter++;
            slashIndex = i;
        }
    }

    // Set path and directory names
    bool blockDirectoryName = false, blockPathName = false;
    for (size_t i = 0; i < sizeof(pathName); i++)
    {
        if (directoriesCounter == 1)
        {
            if ((i + 1) < strlen(filePath.c_str()))
                pathName[i] = filePath.at(i + 1);
            else
                pathName[i] = 0;

            if (filePath.at(i) == '/')
                directoryName[i] = '/';
            else
                directoryName[i] = 0;
        }

        else
        {
            if ((i + 1) < strlen(filePath.c_str()) && filePath.at(i + 1) != '/' && !blockDirectoryName)
                directoryName[i] = filePath.at(i + 1);
            else
            {
                directoryName[i] = 0;
                blockDirectoryName = true;
            }

            if ((slashIndex + 1) < strlen(filePath.c_str()) && filePath.at(slashIndex + 1) != '/' && !blockPathName)
                pathName[i] = filePath.at(slashIndex + 1);
            else
            {
                pathName[i] = 0;
                blockPathName = true;
            }
            slashIndex++;
        }
    }

    char blockValues[sizeOfBlockVector];
    goTo = 3 + sizeOfBitmap + sizeOfInodeVector + 1;
    fseek(file, goTo, SEEK_SET);
    fread(&blockValues, sizeOfChar, sizeof(blockValues), file); // Read the block vector value

    // Size of repozitory files
    if (directoryName[0] != '/')
    {
        goTo = 3 + sizeOfBitmap;
        fseek(file, goTo, SEEK_SET);
        for (int i = 0; i < numInodes; i++)
        {
            fread(&inode, sizeof(INODE), 1, file);
            if (!strcmp(inode.NAME, directoryName))
            {
                char sizeOfDirectory = 0;
                goTo = 3 + sizeOfBitmap + i * sizeof(INODE) + 12;
                fseek(file, goTo, SEEK_SET);
                fread(&sizeOfDirectory, sizeOfChar, 1, file); // Read the size of directory
                fseek(file, -1, SEEK_CUR);
                sizeOfDirectory++;
                fwrite(&sizeOfDirectory, sizeOfChar, 1, file); // Increase the size of directory in 1

                goTo = 3 + sizeOfBitmap + sizeOfInodeVector + 1 + blockSize * inode.DIRECT_BLOCKS[0];
                fseek(file, goTo, SEEK_SET);
                fwrite(&freeInodeIndex, sizeOfChar, 1, file); // Save Inode index on block

                goTo = 3 + sizeOfBitmap + i * sizeof(INODE) + sizeof(INODE);
                fseek(file, goTo, SEEK_SET);
            }
        }
    }

    char freeBlockIndex = 0; // Index of a free block
    goTo = 3 + sizeOfBitmap;
    fseek(file, goTo, SEEK_SET);
    for (int i = 0; i < numInodes; i++)
    {
        fread(&inode, sizeof(INODE), 1, file); // Read inode to count blocks

        for (size_t j = 0; j < sizeof(inode.DIRECT_BLOCKS); j++)
        {
            if (inode.DIRECT_BLOCKS[j] > freeBlockIndex)
            {
                freeBlockIndex = inode.DIRECT_BLOCKS[j];
            }
        }
    }
    freeBlockIndex++;

    char setFileInodeUsed[2] = {0x01, 0x00};                  // Set the inode of file as file and used
    goTo = 3 + sizeOfBitmap + sizeof(INODE) * freeInodeIndex; // Reference to use with fseek

    fseek(file, goTo, SEEK_SET);
    fwrite(&setFileInodeUsed, sizeOfChar, 2, file); // Set as used and as file
    fwrite(&pathName, sizeOfChar, 10, file);        // Add the file name

    int sizeOfContent = strlen(fileContent.c_str());
    fwrite(&sizeOfContent, sizeOfChar, 1, file); // Add the size of the content in the file

    char numberOfBlocks = ceil(sizeOfContent / (double)blockSize);

    for (size_t i = 0; i < numberOfBlocks; i++)
    {
        fwrite(&freeBlockIndex, sizeOfChar, 1, file); // Add blocks id in file starting in 1;
        freeBlockIndex++;
    }

    char contentOfFile[strlen(fileContent.c_str())]; // The content to be saved on file
    for (size_t i = 0; i < strlen(fileContent.c_str()); i++)
    {
        contentOfFile[i] = fileContent.at(i);
    }

    // QUICK FIX to save index and content on blocks
    if (directoryName[0] != '/')
    {
        char directoryBlock[] = {1, 2};
        goTo = 3 + sizeOfBitmap + sizeOfInodeVector + 1;
        fseek(file, goTo, SEEK_SET);
        fwrite(&directoryBlock, sizeOfChar, 2, file); // Add 01 and 02 because of the blocks used by path

        goTo = 3 + sizeOfBitmap + sizeOfInodeVector + 1 + blockSize * (freeBlockIndex - numberOfBlocks);
        fseek(file, goTo, SEEK_SET);
        fwrite(&contentOfFile, sizeOfChar, strlen(fileContent.c_str()), file); // Add the file content in the blocks;
    }

    else
    {
        char directoryBlock[] = {1, 0};
        goTo = 3 + sizeOfBitmap + sizeOfInodeVector + 1;
        fseek(file, goTo, SEEK_SET);
        fwrite(&directoryBlock, sizeOfChar, 2, file);                          // Add 01 and 00 because of block used by directory
        fwrite(&contentOfFile, sizeOfChar, strlen(fileContent.c_str()), file); // Add the file content in the blocks;
    }

    // QUICK FIX to the number of children
    int numberOfChildren = 1;
    if (directoryName[0] != '/')
        numberOfChildren = 2;

    goTo = 3 + sizeOfBitmap + 12;
    fseek(file, goTo, SEEK_SET);
    fwrite(&numberOfChildren, sizeOfChar, 1, file); // Add how many children the directory has

    char numberOfBlocksUsed = 0; // Index of a free block
    goTo = 3 + sizeOfBitmap;
    fseek(file, goTo, SEEK_SET);
    for (int i = 0; i < numInodes; i++)
    {
        fread(&inode, sizeof(INODE), 1, file); // Read inode to count blocks

        for (size_t j = 0; j < sizeof(inode.DIRECT_BLOCKS); j++)
        {
            if (inode.DIRECT_BLOCKS[j] > numberOfBlocksUsed)
            {
                numberOfBlocksUsed = inode.DIRECT_BLOCKS[j];
            }
        }
    }

    int counterOfFilled = 0;
    int valueToHex = 0; // Value of filled blocks in binary to dacimal
    for (size_t i = 0; i <= numberOfBlocksUsed; i++)
    {

        valueToHex += pow(2, counterOfFilled);
        counterOfFilled++;
    }
    fseek(file, 3, SEEK_SET);
    fwrite(&valueToHex, sizeOfChar, 1, file); // Add the number of block filled

    fclose(file); // Close file
}

/**
 * @brief Adiciona um novo diretório dentro do sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
 * @param fsFileName arquivo que contém um sistema sistema de arquivos que simula EXT3.
 * @param dirPath caminho completo novo diretório dentro sistema de arquivos que simula EXT3.
 */
void addDir(std::string fsFileName, std::string dirPath)
{
    FILE *file = fopen(fsFileName.c_str(), "r+");

    char tempValues[3];
    int blockSize, numBlocks, numInodes;
    size_t sizeOfChar = sizeof(char); // Size of a char

    fread(&tempValues, sizeOfChar, 3, file); // Read block size, number, blocks and number of inodes and bitmap
    blockSize = tempValues[0];
    numBlocks = tempValues[1];
    numInodes = tempValues[2];

    int sizeOfInodeVector = numInodes * sizeof(INODE); // Size of the Inode Vector (-1 because of root)
    int sizeOfBlockVector = numBlocks * blockSize;     // Size of the Block Vector
    char sizeOfBitmap = ceil((numBlocks - 1) / 8.0);   // Size of the Bitmap (-1 because of root)

    char blockValues[sizeOfBlockVector];
    int goTo = 3 + sizeOfBitmap + sizeOfInodeVector + 1;
    fseek(file, goTo, SEEK_SET);
    fread(&blockValues, sizeOfChar, sizeof(blockValues), file); // Read the block vector value

    int freeBlockIndex; // Index of a free block
    for (size_t i = 0; i < sizeof(blockValues); i += blockSize)
    {
        if (blockValues[i] == 0)
        {
            freeBlockIndex = i / 2;
            break;
        }
    }

    INODE inode;
    int freeInodeIndex; // Index of a free inode
    goTo = 3 + sizeOfBitmap;
    fseek(file, goTo, SEEK_SET);
    for (int i = 0; i < numInodes; i++)
    {
        fread(&inode, sizeof(INODE), 1, file); // Read inode to find an empty one

        if (inode.IS_USED == 0)
        {
            freeInodeIndex = i;
            break;
        }
    }

    char dirName[10]; // Directory path name (skip "/")
    for (size_t i = 0; i < sizeof(dirName); i++)
    {
        if (i + 1 < strlen(dirPath.c_str()) && i + 1 < strlen(dirPath.c_str()))
            dirName[i] = dirPath.at(i + 1);
        else
            dirName[i] = 0;
    }

    goTo = 3 + sizeOfBitmap + freeInodeIndex * sizeof(INODE);
    char setDirectory[] = {0x01, 0x01};
    fseek(file, goTo, SEEK_SET);
    fwrite(&setDirectory, sizeOfChar, 2, file); // Set as directory
    fwrite(&dirName, sizeOfChar, 10, file);     // Set the name of directory
    fseek(file, 1, SEEK_CUR);
    fwrite(&freeBlockIndex, sizeOfChar, 1, file); // Set the index of block

    int numberOfBlocks = 1;
    goTo = 3 + sizeOfBitmap;
    fseek(file, goTo, SEEK_SET);
    int numberOfInodesFilled = -1;
    for (int i = 0; i < numInodes; i++)
    {
        fread(&inode, sizeof(INODE), 1, file); // Read inode to find how much blocks there are
        if (inode.IS_USED == 1)
        {
            numberOfInodesFilled++;

            if (inode.DIRECT_BLOCKS[0] > 0)
                numberOfBlocks++;
            if (inode.DIRECT_BLOCKS[1] > 0)
                numberOfBlocks++;
            if (inode.DIRECT_BLOCKS[2] > 0)
                numberOfBlocks++;
        }
    }

    int bitMap = 0;
    for (size_t i = 0; i < numberOfBlocks; i++)
    {
        bitMap += pow(2, i);
    }

    fseek(file, 3, SEEK_SET);
    fwrite(&bitMap, sizeOfChar, 1, file); // Add the bitmap

    goTo = 3 + sizeOfBitmap + 12;
    fseek(file, goTo, SEEK_SET);
    fwrite(&numberOfInodesFilled, sizeOfChar, 1, file); // Add the number of inodes in directory

    int valueInRootBlock = 2;
    goTo = 3 + sizeOfBitmap + sizeOfInodeVector + 2;
    fseek(file, goTo, SEEK_SET);
    fwrite(&valueInRootBlock, sizeOfChar, 1, file); // Add the number of inodes in directory block

    fclose(file);
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
